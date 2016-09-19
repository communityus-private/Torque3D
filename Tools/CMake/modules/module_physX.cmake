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

# PhysX module

option(TORQUE_PHYSICS_PHYSX "Use PhysX physics" OFF)

if( NOT TORQUE_PHYSICS_PHYSX )
    return()
endif()
	           
addDef( "TORQUE_PHYSICS_PHYSX" )
addDef( "TORQUE_PHYSICS_ENABLED" )

addLib( LowLevel )
addLib( LowLevelCloth )
addLib( PhysX SHARED )

addPath( "${srcDir}/T3D/physics/physx3" )
addInclude( "${libDir}/physX/include" )
addInclude( "${libDir}/physX/include/characterkinematic" )
addInclude( "${libDir}/physX/include/cloth" )
addInclude( "${libDir}/physX/include/common" )
addInclude( "${libDir}/physX/include/cooking" )
addInclude( "${libDir}/physX/include/foundation" )
addInclude( "${libDir}/physX/include/geometry" )
addInclude( "${libDir}/physX/include/vehicle" )
