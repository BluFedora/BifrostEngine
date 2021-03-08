cloc-1.76 --by-file --csv --exclude-dir=lib ..\Experiments\ -out=code_count_bifrost_experiments.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\Engine\Graphics\ -out=code_count_bifrost_graphics.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\Engine\Math\ -out=code_count_bifrost_math.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\Engine\Memory\ -out=code_count_bifrost_memory.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\Engine\Platform\ -out=code_count_bifrost_platform.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\Engine\Runtime\ -out=code_count_bifrost_runtime.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostScript\ -out=code_count_bifrost_script.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostUtility\ -out=code_count_bifrost_utility.csv

cloc-1.76 --by-file --csv --exclude-dir=lib ^
..\BifrostScript\ ^
..\Experiments\ ^
..\Engine\Anim2D\ ^
..\Engine\AssetIO\ ^
..\Engine\Audio\ ^
..\Engine\Core\ ^
..\Engine\DataStructures\ ^
..\Engine\Dispatch\ ^
..\Engine\Editor\ ^
..\Engine\Graphics\ ^
..\Engine\Job\ ^
..\Engine\Math\ ^
..\Engine\Memory\ ^
..\Engine\Meta\ ^
..\Engine\Network\ ^
..\Engine\Platform\ ^
..\Engine\Runtime\ ^
..\Engine\Text\ ^
..\Engine\TMPUtils\ ^
..\Engine\UI\ ^
..\Runtime\ ^
-out=code_count_engine_total.csv
