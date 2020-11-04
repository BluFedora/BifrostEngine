cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostExperiments\ -out=code_count_bifrost_experiments.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostGraphics\ -out=code_count_bifrost_graphics.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\Engine\Math\ -out=code_count_bifrost_math.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\Engine\Memory\ -out=code_count_bifrost_memory.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostMeta\ -out=code_count_bifrost_meta.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\Engine\Platform\ -out=code_count_bifrost_platform.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\Engine\Runtime\ -out=code_count_bifrost_runtime.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostScript\ -out=code_count_bifrost_script.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostUtility\ -out=code_count_bifrost_utility.csv

cloc-1.76 --by-file --csv --exclude-dir=lib ^
..\BifrostExperiments\ ^
..\BifrostGraphics\ ^
..\Engine\Math\ ^
..\Engine\Memory\ ^
..\BifrostMeta\ ^
..\Engine\Platform\ ^
..\BifrostRuntime\ ^
..\Engine\Runtime\ ^
..\BifrostScript\ ^
..\BifrostUtility\ ^
..\Bifrost\ ^
-out=code_count_bifrost_total.csv
