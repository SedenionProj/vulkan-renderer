C:/VulkanSDK/1.4.313.0/Bin/glslc.exe -fshader-stage=vert basic.vert -o basicVert.spv
C:/VulkanSDK/1.4.313.0/Bin/glslc.exe -fshader-stage=frag basic.frag -o basicFrag.spv

C:/VulkanSDK/1.4.313.0/Bin/glslc.exe -fshader-stage=frag postProcess.frag -o postProcessFrag.spv

C:/VulkanSDK/1.4.313.0/Bin/glslc.exe -fshader-stage=vert cubemap.vert -o cubemapVert.spv
C:/VulkanSDK/1.4.313.0/Bin/glslc.exe -fshader-stage=frag cubemap.frag -o cubemapFrag.spv

C:/VulkanSDK/1.4.313.0/Bin/glslc.exe -fshader-stage=vert shadowMap.vert -o shadowMapVert.spv
C:/VulkanSDK/1.4.313.0/Bin/glslc.exe -fshader-stage=frag shadowMap.frag -o shadowMapFrag.spv

C:/VulkanSDK/1.4.313.0/Bin/glslc.exe -fshader-stage=vert depthPrePass.vert -o depthPrePassVert.spv
C:/VulkanSDK/1.4.313.0/Bin/glslc.exe -fshader-stage=frag depthPrePass.frag -o depthPrePassFrag.spv

C:/VulkanSDK/1.4.313.0/Bin/glslc.exe -fshader-stage=frag ssao.frag -o ssaoFrag.spv

C:/VulkanSDK/1.4.313.0/Bin/glslc.exe -fshader-stage=vert screen.vert -o screenVert.spv


pause