echo "Input: Solution-dir output-version exename"
%1..\bin\masp_repl.exe %1..\data\scripts\pathecho.mp %1../data %1..\bin\config.mp
copy %1\%2\%3 %1..\bin\%3
copy %1..\lib_ext\glew32.dll %1..\bin\glew32.dll
