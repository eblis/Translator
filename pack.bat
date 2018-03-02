for /F "tokens=4-8* delims=. " %%i in (docs\Translator_readme.txt) do (call :Pack %%i %%j %%k %%l; exit)

:Pack
d:\usr\PowerArchiver\pacl\pacomp.exe -a -c2 "Translator %1.%2.%3.%4 x32.zip" @files_release.txt
d:\usr\PowerArchiver\pacl\pacomp.exe -p -a -c2 "Translator %1.%2.%3.%4 x32.zip" docs\*.txt *.caca
call "pack symbols.bat" Translator Translator %1.%2.%3.%4
exit

error:
echo "Error packing Translator"
