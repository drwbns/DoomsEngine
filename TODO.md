TO-DO List            

### Code Refactoring
When I read the code of unreal engine4, I thought my engine's code readability is really horrible. I should fix this!      

### Read back results of culling one frame later         
        
### Support D3D11, OpenGL completely

### Implement Hierarchical Shadow map

### Batch draw    
Fix up draw order      

### Reduce header dependencies ( Reduce build time )

### Implement config value setting window

### Rebuild clscan so paths with spaces work
Right now the project cannot live under a path containing spaces. `clscan.dll`
splits its command line on spaces *before* honouring quotes, so a quoted path
comes back with the quote character embedded in it:

    error: no such file or directory: 'Projects\Dooms-latest\Doom3\ThirdParty\nlohmann"'

Under `E:\source\Graphics Engines and Projects\Dooms-latest` this produced 127130
`no such file or directory` errors and 132 clScan failures. The identical build run
through a space-free `subst` drive produced zero errors, clScan 132/132, clMerge and
clExport success, and a correctly regenerated `reflection_binary_Debug_x64.cppbin`.

`clscan.dll` is a prebuilt binary (Feb 2024), so this cannot be fixed from the engine
or from `clReflect_automation`; quoting was tried at both layers and reverted. The fix
has to be in clscan's own argument tokenizer.

To do it:
  * Source is at `SungJJinKang/clReflect_ForDoomsEngine`, branch `doom_engine_version`
    (commit `ccebec26`) — the one upstream repo that still exists. A vendored copy is
    already in the repo root as `clReflect_ForDoomsEngine-doom_engine_version/`.
  * Fix the tokenizer to honour double quotes, then rebuild `clscan.dll` and drop it
    into `Doom3/x64/<Config>/`. It is an LLVM/clang-based build, so expect it to be
    a large one.
  * Once clscan honours quotes, restore the quoting in
    `clReflect_automation/ParseAdditionalDirectories.cs` and
    `clReflectCaller.GenerateClScanArguments` (both currently carry comments
    explaining why they are deliberately unquoted).

Until then, keep the project under a path with no spaces — and note this must apply at
**build** time, not just at run time. `VCXPROJ_PATH` is baked into the binary at compile
time via `-DVCXPROJ_PATH="$(ProjectPath)"`, and clReflect derives every source file path
from it. An engine built under a spaced path but launched from a `subst`'d space-free
drive still reports `E:\source\Graphics Engines and Projects\...` and still fails, so a
permanent move beats a runtime `subst`.

Note the separate, unrelated constraint: the prebuilt clscan uses clang 12.0.1, which
cannot parse the VS2022 runtime headers, so the engine must be built with VS2019 /
v142 and `VCToolsInstallDir` pointed at the VS2019 tools directory.
