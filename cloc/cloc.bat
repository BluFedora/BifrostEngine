rem cloc-1.76 --by-file --csv --exclude-dir=lib --force-lang-def=cloc_definitions.txt ..\BifrostExperiments\ -out=code_count_bifrost_experiments.csv
rem cloc-1.76 --by-file --csv --exclude-dir=lib --force-lang-def=cloc_definitions.txt ..\BifrostGraphics\ -out=code_count_bifrost_graphics.csv
rem cloc-1.76 --by-file --csv --exclude-dir=lib --force-lang-def=cloc_definitions.txt ..\BifrostMath\ -out=code_count_bifrost_math.csv
rem cloc-1.76 --by-file --csv --exclude-dir=lib --force-lang-def=cloc_definitions.txt ..\BifrostMemory\ -out=code_count_bifrost_memory.csv
rem cloc-1.76 --by-file --csv --exclude-dir=lib --force-lang-def=cloc_definitions.txt ..\BifrostMeta\ -out=code_count_bifrost_meta.csv
rem cloc-1.76 --by-file --csv --exclude-dir=lib --force-lang-def=cloc_definitions.txt ..\BifrostPlatform\ -out=code_count_bifrost_platform.csv
rem cloc-1.76 --by-file --csv --exclude-dir=lib --force-lang-def=cloc_definitions.txt ..\BifrostRuntime\ -out=code_count_bifrost_runtime.csv
rem cloc-1.76 --by-file --csv --exclude-dir=lib --force-lang-def=cloc_definitions.txt ..\BifrostScript\ -out=code_count_bifrost_script.csv
rem cloc-1.76 --by-file --csv --exclude-dir=lib --force-lang-def=cloc_definitions.txt ..\BifrostUtility\ -out=code_count_bifrost_utility.csv

cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostExperiments\ -out=code_count_bifrost_experiments.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostGraphics\ -out=code_count_bifrost_graphics.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostMath\ -out=code_count_bifrost_math.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostMemory\ -out=code_count_bifrost_memory.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostMeta\ -out=code_count_bifrost_meta.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostPlatform\ -out=code_count_bifrost_platform.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostRuntime\ -out=code_count_bifrost_runtime.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostScript\ -out=code_count_bifrost_script.csv
cloc-1.76 --by-file --csv --exclude-dir=lib ..\BifrostUtility\ -out=code_count_bifrost_utility.csv

cloc-1.76 --by-file --csv --exclude-dir=lib ^
..\BifrostExperiments\ ^
..\BifrostGraphics\ ^
..\BifrostMath\ ^
..\BifrostMemory\ ^
..\BifrostMeta\ ^
..\BifrostPlatform\ ^
..\BifrostRuntime\ ^
..\BifrostScript\ ^
..\BifrostUtility\ ^
..\Bifrost\ ^
-out=code_count_bifrost_total.csv
