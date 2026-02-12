## Requirements
* [CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR/tree/ng))
	* Add the environment variable `COMMONLIBSSE_PATH` with the value as the path to the folder containing CommonLibSSE-NG
* [_ts_SKSEFunctions](https://github.com/staalo18/TS_SKSEFunctions))
	* Install this into a directory parallel to the FCFW project directory. You'll need to update the path in  CMakeLists.txt accordingly.
* [powerofthree's CLIB Utils](https://github.com/powerof3/CLibUtil/tree/master))
	* Install this into a directory parallel to the FCFW project directory. You'll need to update the path in  CMakeLists.txt accordingly.
	* Mods and SKSE plugins using FCFW require powerofthree's Tweaks in the mod list (https://www.nexusmods.com/skyrimspecialedition/mods/51073)!
* Change OUTPUT_FOLDER variable in CMakeLists.txt to point to your local path for where the generated DLL should be copied to.

