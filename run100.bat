xcopy .\bin\x64\Debug\Thesis.exe Thesis.exe /Y
xcopy .\bin\x64\Debug\ComputeShader.cso ComputeShader.cso /Y

FOR /L %%i IN (0, 1, 99) DO start /b /wait "" ".\Thesis.exe" 