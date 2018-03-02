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

#ifndef M_TRANSLATOR_LIST_H
#define M_TRANSLATOR_LIST_H

#define INITIAL_SIZE 50

#include "commonheaders.h"

#define FILTER_ALL          0
#define FILTER_UNTRANSLATED 1
#define FILTER_TRANSLATED   2

extern int nCurrentFilterMode;

#define TF_DEFAULT   0
#define TF_DIALOG    1
#define TF_MENU      2
#define TF_LANGPACK  4
#define TF_PREVIOUS  8
#define TF_WIDE     16

struct TDynamicTranslation{
	char *szDynamic;
	wchar_t *wszDynamic;
};

typedef TDynamicTranslation *PDynamicTranslation;

struct TTranslation{
	wchar_t *english;
	wchar_t *translation;
	wchar_t *module;
	DWORD hEnglish;
	DWORD hTranslation;
	DWORD hModule;
	DWORD flags;
	PDynamicTranslation dynamic;
	
	TTranslation(const wchar_t *english, const wchar_t *translation, const wchar_t *module, DWORD hEnglish, DWORD hTranslation, DWORD hModule, DWORD flags, PDynamicTranslation dynamic)
	{
		this->english = _wcsdup(english);
		this->translation = _wcsdup(translation);
		this->module = _wcsdup(module);
		this->hEnglish = hEnglish;
		this->hTranslation = hTranslation;
		this->hModule = hModule;
		this->flags = flags;
		this->dynamic = dynamic;
	}
	
	~TTranslation()
	{
		free(this->english);
		free(this->translation);
		free(this->module);
		if (dynamic)
		{
			if (dynamic->szDynamic)
			{
				free(dynamic->szDynamic);
			}
			
			if (dynamic->wszDynamic)
			{
				free(dynamic->wszDynamic);
			}
			
			free(dynamic);
		}
	}
};

typedef TTranslation *PTranslation;

PDynamicTranslation BuildDynamicTranslation(const wchar_t *translation);
void ReplaceDynamicTranslation(PTranslation item, const wchar_t *translation);

inline void ReplaceTranslationString(wchar_t *&toReplace, const wchar_t *replaceWith)
{
	free(toReplace);
	toReplace = (replaceWith) ? _wcsdup(replaceWith) : NULL;
}

class CTranslationsList{
	protected:
		PTranslation *_translations;
		long _count;
		long _capacity;
		int _translationsMode;
		int _ignoreDoubleTranslations;
		int _dynamicTranslations;
		
		void Enlarge(int increaseAmount);
		void EnsureCapacity();		
	
	public:
		CTranslationsList(int initialSize = INITIAL_SIZE);
		~CTranslationsList();

		void Clear();
		
		long Add(const wchar_t *english, const wchar_t *translation, const wchar_t *module, DWORD flags, int bReplaceTranslation = 0);
		int Remove(int index);
		int Contains(const wchar_t *english, const wchar_t *module) const;
		long Index(const wchar_t *english, const wchar_t *module) const;
		
		int TranslationExists(DWORD hTranslation);
		
		const PTranslation operator [](int index);
		
		void Flush(int bDelete = 1);
		
		void InvalidateTranslation(TTranslation *translation);
		
		void SetTranslationsMode(int translationsMode);
		int TranslationsMode();
		
		void Sort();
		
		void SetIgnoreDoubleTranslations(int ignoreDoubleTranslations);
		int IgnoreDoubleTranslations();
		
		void SetDynamicTranslations(int dynamicTranslations);
		int DynamicTranslations();
		
		long Count() const;
		long Capacity() const;
};

extern CTranslationsList &lstTranslations; //list of translations

#endif //M_TRANSLATOR_LIST_H