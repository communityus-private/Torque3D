# -----------------------------------------------------------------------------
# Copyright (c) 2015 GarageGames, LLC
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
# -----------------------------------------------------------------------------

project(LowLevel)
addInclude("${libDir}/physX/externals/nvToolsExt/1/include")
addInclude("${libDir}/physX/externals/nvToolsExt/1/include/stdint")
addInclude("${libDir}/physX/Include/foundation")
addInclude("${libDir}/physX/Source/foundation/include")
addInclude("${libDir}/physX/Include/physxprofilesdk")
addInclude("${libDir}/physX/Include/physxvisualdebuggersdk")
addInclude("${libDir}/physX/Include/common")
addInclude("${libDir}/physX/Include/geometry")
addInclude("${libDir}/physX/Include/pxtask")
addInclude("${libDir}/physX/Include")
addInclude("${libDir}/physX/Source/Common/src")
addInclude("${libDir}/physX/Source/PhysXProfile/src")
addInclude("${libDir}/physX/Source/PhysXProfile/include")
addInclude("${libDir}/physX/Source/GeomUtils/headers")
addInclude("${libDir}/physX/Source/GeomUtils/src/contact")
addInclude("${libDir}/physX/Source/GeomUtils/src/common")
addInclude("${libDir}/physX/Source/GeomUtils/src/convex")
addInclude("${libDir}/physX/Source/GeomUtils/src/distance")
addInclude("${libDir}/physX/Source/GeomUtils/src/gjk")
addInclude("${libDir}/physX/Source/GeomUtils/src/intersection")
addInclude("${libDir}/physX/Source/GeomUtils/src/mesh")
addInclude("${libDir}/physX/Source/GeomUtils/src/hf")
addInclude("${libDir}/physX/Source/GeomUtils/src/pcm")
addInclude("${libDir}/physX/Source/GeomUtils/src/")

addInclude("${libDir}/physX/Source/LowLevel/API/include" )
addInclude("${libDir}/physX/Source/LowLevel/common/include/collision" )
addInclude("${libDir}/physX/Source/LowLevel/common/include/pipeline" )

addInclude("${libDir}/physX/Source/LowLevel/common/include/math" )
addInclude("${libDir}/physX/Source/LowLevel/common/include/utils" )
addInclude("${libDir}/physX/Source/LowLevel/software/include" )

if(WIN32)
    addInclude("${libDir}/physX/Source/Common/src/windows")
    addInclude("${libDir}/physX/Source/LowLevel/common/include/pipeline/windows" )
    addInclude("${libDir}/physX/Source/LowLevel/software/include/windows" )
endif()

addPath("${libDir}/physX/Source/LowLevel/API/src" )
if(WIN32)
    addPath("${libDir}/physX/Source/LowLevel/API/src/windows" )
else()
    addPath("${libDir}/physX/Source/LowLevel/API/src/linux" )
endif()

addPath("${libDir}/physX/Source/LowLevel/software/src" )

if(WIN32)
    addPath("${libDir}/physX/Source/LowLevel/API/src/windows" )
endif()

#ADD_LIBRARY(nvToolsExtLib STATIC IMPORTED)
#SET_TARGET_PROPERTIES(nvToolsExtLib PROPERTIES
#    IMPORTED_LOCATION ${libDir}/physX/externals/nvToolsExt/1/lib/Win32/nvToolsExt32_1.lib)
#TARGET_LINK_LIBRARIES(nvToolsExtLib)

#add_library( nvToolsExt32_1.lib STATIC IMPORTED )
#set_target_properties( nvToolsExtLib PROPERTIES IMPORTED_LOCATION ${libDir}/physX/externals/nvToolsExt/1/lib/Win32/ )
#target_link_libraries(nvToolsLib )

finishLibrary()