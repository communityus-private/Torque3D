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

project(PhysX)
addInclude("${libDir}/physX/externals/nvToolsExt/1/include")
addInclude("${libDir}/physX/externals/nvToolsExt/1/include/stdint")
addInclude("${libDir}/physX/Include/foundation")
addInclude("${libDir}/physX/Source/foundation/include")
addInclude("${libDir}/physX/Include/physxprofilesdk")
addInclude("${libDir}/physX/Include/physxvisualdebuggersdk")
addInclude("${libDir}/physX/Include/common")
addInclude("${libDir}/physX/Include/geometry")
addInclude("${libDir}/physX/Include/pvd")
addInclude("${libDir}/physX/Include/particles")
addInclude("${libDir}/physX/Include/cloth")
addInclude("${libDir}/physX/Include/pxtask")
addInclude("${libDir}/physX/Include")
addInclude("${libDir}/physX/Source/Common/src")
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
addInclude("${libDir}/physX/Source/SimulationController/include" )
addInclude("${libDir}/physX/Source/PhysXCooking/src" )
addInclude("${libDir}/physX/Source/PhysXCooking/src/mesh" )
addInclude("${libDir}/physX/Source/PhysXCooking/src/convex" )
addInclude("${libDir}/physX/Source/SceneQuery" )
addInclude("${libDir}/physX/Source/PvdRuntime/src" )
addInclude("${libDir}/physX/Source/PhysXMetaData/core/include" )

addPath("${libDir}/physX/Source/PhysX/src" )
addPath("${libDir}/physX/Source/PhysX/src/buffering" )
addPath("${libDir}/physX/Source/PhysX/src/cloth" )
addPath("${libDir}/physX/Source/PhysX/src/device" )
addPath("${libDir}/physX/Source/PhysX/src/gpu" )
addPath("${libDir}/physX/Source/PhysX/src/particles" )

if(WIN32)
    addPath("${libDir}/physX/Source/PhysX/src/windows" )
endif()

finishLibrary()