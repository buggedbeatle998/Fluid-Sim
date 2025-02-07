@echo off
for %%a in (%*) do C:\VulkanSDK\1.3.296.0\Bin\glslc.exe %%a -o %%~dpa\..\shaders\%%~nxa.spv
pause