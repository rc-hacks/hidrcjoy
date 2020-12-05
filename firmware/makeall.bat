@echo off

call make.bat %* BOARD=Digispark clean all
copy objs\hidrcjoy.hex ..\releases\hidjoy_digispark.hex

call make.bat %* BOARD=DigisparkPro clean all
copy objs\hidrcjoy.hex ..\releases\hidjoy_digisparkpro.hex

call make.bat %* BOARD=FabISP clean all
copy objs\hidrcjoy.hex ..\releases\hidjoy_fabisp.hex

call make.bat %* BOARD=ProMicro clean all
copy objs\hidrcjoy.hex ..\releases\hidjoy_promicro.hex
