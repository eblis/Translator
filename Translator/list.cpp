/*
Translator plugin for Miranda IM

Copyright © 2006-2007 Cristian Libotean

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "list.h"

int nCurrentFilterMode = 0;

struct TBinarySearch{
	DWORD hEnglish;
	DWORD hModule;
};

typedef TBinarySearch *PBinarySearch;

CRITICAL_SECTION CRITICALSECTION_SORT;

static inline int BinarySearch(const void *key, const void *element)
{
	PBinarySearch search = (PBinarySearch) key;
	PTranslation translation = *(PTranslation *) element;
	
	if (search->hEnglish == translation->hEnglish) //translation hashes are equal, need to compare module hash too
	{
		if ((search->hModule == translation->hModule) || (search->hModule == 0) || (translation->hModule == 0))
		{
			return 0;
		}
		
		return (search->hModule < translation->hModule) ? -1 : 1;
	}
	else{
		return (search->hEnglish < translation->hEnglish) ? -1 : 1;
	}
}

static inline int BinarySearchExists(const void *key, const void *element)
{
	DWORD hash = (DWORD) key;
	PTranslation translation = *(PTranslation *) element;
	
	if (translation == NULL)
	{
		return (hash == NULL) ? 0 : 1;
	}
	
	if (hash < translation->hEnglish)
	{
		return -1;
	}
	
	return (hash == translation->hEnglish) ? 0 : 1;
}

static inline int QuickSort(const void *key, const void *element)
{
	PTranslation first  = *(PTranslation *) key;
	PTranslation second =  *(PTranslation *) element;
	
	if ((first == NULL) || (second == NULL))
	{
		return (first == NULL) ? -1 : 1;
	}
	
	if (first->hEnglish == second->hEnglish) //translation hashes are equal, need to compare module hash too
	{
		if ((first->hModule == second->hModule) || (first->hModule == 0) || (second->hModule == 0))
		{
			return 0;
		}
		
		return (first->hModule < second->hModule) ? -1 : 1;
	}
	else{
		return (first->hEnglish < second->hEnglish) ? -1 : 1;
	}
}

CTranslationsList &lstTranslations = CTranslationsList();

PDynamicTranslation BuildDynamicTranslation(const wchar_t *translation)
{
	PDynamicTranslation dynamic = (PDynamicTranslation) malloc(sizeof(TDynamicTranslation));
	wchar_t *wszDynamic = _wcsdup(translation);
	ParseSpecialChars(wszDynamic);
	size_t size = wcslen(wszDynamic) + 1;
	char *szDynamic = (char *) malloc(size);
	WideCharToMultiByte(ACTIVE_CODEPAGE, 0, wszDynamic, -1, szDynamic, (int) size, NULL, NULL);
	dynamic->wszDynamic = wszDynamic;
	dynamic->szDynamic = szDynamic;
	
	return dynamic;
}

void ReplaceDynamicTranslation(PTranslation item, const wchar_t *translation)
{
	if ((item) && (item->dynamic))
	{
		if (item->dynamic->szDynamic)
		{
			free(item->dynamic->szDynamic);
		}
		
		if (item->dynamic->wszDynamic)
		{
			free(item->dynamic->wszDynamic);
		}
		
		free(item->dynamic);
		
		item->dynamic = BuildDynamicTranslation(translation);
	}
	
	//qsort(lstTranslations.Translations(), lstTranslations.Count(), sizeof(PTranslation), QuickSort); //resort list
}


CTranslationsList::CTranslationsList(int initialCapacity)
{
	_translations = NULL;
	_count = 0;
	_capacity = 0;
	
	InitializeCriticalSection(&CRITICALSECTION_SORT);
	
	Enlarge(initialCapacity);
}

CTranslationsList::~CTranslationsList()
{
	Clear();
	free(_translations);
	
	DeleteCriticalSection(&CRITICALSECTION_SORT);
}

void CTranslationsList::Clear()
{
	long i;
	for (i = 0; i < Count(); i++)
	{
		delete _translations[i];
	}
	_count = 0;
}

long CTranslationsList::Count() const
{
	return _count;
}

long CTranslationsList::Capacity() const
{
	return _capacity;
}

void CTranslationsList::EnsureCapacity()
{
	
	if (_count >= _capacity)
		{
			int flushLimit = DBGetContactSettingWord(NULL, ModuleName, "FlushLimit", 200);
			if ((_capacity < flushLimit) || (!bModulesLoaded) || (TranslationsMode()))
			{
				Enlarge(_capacity / 2);
			}
			else{
				Flush();
			}
		}
}

void CTranslationsList::Enlarge(int increaseAmount)
{
	int newSize = _capacity + increaseAmount;
	_translations = (PTranslation *) realloc(_translations, newSize * sizeof(PTranslation));
	_capacity = newSize;
}

int CTranslationsList::Contains(const wchar_t *english, const wchar_t *module) const
{
	int pos = Index(english, module);
	return (pos >= 0);
}

long CTranslationsList::Index(const wchar_t *english, const wchar_t *module) const
{
	DWORD hash = LangPackHashW(english);
	DWORD moduleHash = LangPackHashW(module);
	
	TBinarySearch search = {hash, moduleHash};
	PTranslation *translation = (PTranslation *) bsearch(&search, _translations, Count(), sizeof(PTranslation), BinarySearch);	
	
	return (translation != NULL) ? translation - _translations : -1;
}

int CTranslationsList::TranslationExists(DWORD hTranslation)
{
	PTranslation *translation = (PTranslation *) bsearch((void *) hTranslation, _translations, Count(), sizeof(PTranslation), BinarySearchExists);
	
	return (translation != NULL);
}

long CTranslationsList::Add(const wchar_t *english, const wchar_t *translation, const wchar_t *module, DWORD flags, int bReplaceTranslation)
{
	long exists = Index(english, module);

	if (exists < 0)
	{
		DWORD hEnglish = LangPackHashW(english);
		DWORD hTranslation = LangPackHashW(translation);
		
		int ok = 1;
		if (!TranslationsMode()) //only perform filtering when flushing is enabled (not in translations mode)
		{
			switch (nCurrentFilterMode)
			{
				case FILTER_UNTRANSLATED:
				{
					ok = (hEnglish == hTranslation) ? 0 : 1;
					
					break;
				}
				
				case FILTER_TRANSLATED:
				{
					ok = (hEnglish == hTranslation) ? 1 : 0;
					
					break;
				}
			}
		}
		else{ // in Translation mode
			if ((IgnoreDoubleTranslations()) && (TranslationExists(hEnglish)) && (hEnglish == hTranslation))
			{
				return -1; //don't add the translation as a separate entry.
			}
		}
		
		if (ok)
		{
			EnsureCapacity();
			PDynamicTranslation dynamic = (DynamicTranslations()) ? BuildDynamicTranslation(translation) : NULL;
			
			
			PTranslation t = new TTranslation(english, translation, module, LangPackHashW(english), LangPackHashW(translation), LangPackHashW(module), flags, dynamic);
			_translations[_count++] = t;
			Sort();
			exists = Index(english, module); //get the index of the item
		}
	}
	else{
		if (bReplaceTranslation) //replace the translation currently in the database ?
		{
			PTranslation item = _translations[exists];
			ReplaceTranslationString(item->translation, translation);
			if (DynamicTranslations())
			{
				ReplaceDynamicTranslation(item, translation);
			}
		}
		if (_translations[exists]->hModule == 0) //module not known yet, assign current module.
		{
			PTranslation item = _translations[exists];
			ReplaceTranslationString(item->module, module);
			item->hModule = LangPackHashW(module);
			if (DynamicTranslations())
			{
				ReplaceDynamicTranslation(item, translation);
			}
		}
		
		_translations[exists]->flags |= flags; //add flags (if any)
	}
	
	return exists; //returns the index of the item or -1 if it wasn't added.
}

int CTranslationsList::Remove(int index)
{
	if ((index < 0) && (index >= Count()))
	{
		return 1;
	}
	
	int i;
	PTranslation tmp = _translations[index];
	for (i = index; i < _count - 1; i++)
	{
		_translations[i] = _translations[i + 1];
	}
	_count--;
	delete tmp;
	
	return 0;
}

const PTranslation CTranslationsList::operator [](int index)
{
	if ((index < 0) || (index >= Count()))
	{
		return NULL;
	}
	
	return _translations[index];
}

void CTranslationsList::Flush(int bDelete)
{
	int i;
	WCHAR fileName[512];
	WCHAR *p;
	
	CreateDirectoryW(TRANSLATIONS_FOLDER, NULL);
	wcscpy(fileName, TRANSLATIONS_FOLDER);

	wcscat(fileName, L"\\");
	p = fileName + wcslen(fileName);
	
	PTranslation translation;
	for (i = 0; i < _count; i++)
	{
		translation = _translations[i];
		
		wcscpy(p, translation->module);
		wcscat(p, L".txt");
		
		if (wcslen(translation->module) > 0) //don't write strings that are in the language pack and haven't been modified
		{
			WriteTranslation(translation->english, translation->translation, fileName);
		}
		//WritePrivateProfileStringW(translation->english, translation->translation /*translation->english*/, L""/*translation->translation*/, fileName);
		if (bDelete)
		{
			delete _translations[i];
		}
	}
	
	if (bDelete)
	{
		_count = 0;
	}
}

void CTranslationsList::InvalidateTranslation(TTranslation *translation)
{
	WCHAR fileName[1024];
	WCHAR backup[1024];
	wcscpy(fileName, TRANSLATIONS_FOLDER);
	wcscat(fileName, L"\\");
	wcscat(fileName, translation->module);
	wcscat(fileName, L".txt");
	wcscpy(backup, fileName);
	wcscat(backup, L".bak");
	
	//DeleteFileW(fileName);
	MoveFileExW(fileName, backup, MOVEFILE_REPLACE_EXISTING);
}

void CTranslationsList::SetTranslationsMode(int translationsMode)
{
	_translationsMode = translationsMode;
}

int CTranslationsList::TranslationsMode()
{
	return _translationsMode;
}

void CTranslationsList::SetIgnoreDoubleTranslations(int ignoreDoubleTranslations)
{
	_ignoreDoubleTranslations = ignoreDoubleTranslations;
}

int CTranslationsList::IgnoreDoubleTranslations()
{
	return _ignoreDoubleTranslations;
}

void CTranslationsList::SetDynamicTranslations(int dynamicTranslations)
{
	_dynamicTranslations = dynamicTranslations;
}

int CTranslationsList::DynamicTranslations()
{
	return _dynamicTranslations && _translationsMode;
}

void CTranslationsList::Sort()
{
	EnterCriticalSection(&CRITICALSECTION_SORT);
	qsort(_translations, Count(), sizeof(PTranslation), QuickSort);
	LeaveCriticalSection(&CRITICALSECTION_SORT);
}