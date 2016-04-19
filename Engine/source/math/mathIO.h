//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _MATHIO_H_
#define _MATHIO_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _STREAM_H_
#include "core/stream/stream.h"
#endif
#ifndef _MMATH_H_
#include "math/mMath.h"
#endif
#ifndef _BITSTREAM_H_
#include "core/stream/bitStream.h"
#endif

#define MATH_PRECISION 32
//------------------------------------------------------------------------------
//-------------------------------------- READING
//
inline bool mathRead(BitStream& stream, Point2I* p, S32 bitCount = MATH_PRECISION)
{
   p->x = stream.readSignedInt(bitCount);
   p->y = stream.readSignedInt(bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, Point3I* p, S32 bitCount = MATH_PRECISION)
{
   p->x = stream.readSignedInt(bitCount);
   p->y = stream.readSignedInt(bitCount);
   p->z = stream.readSignedInt(bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, Point2F* p, S32 bitCount = MATH_PRECISION)
{
   p->x = stream.readSignedFloat(bitCount);
   p->y = stream.readSignedFloat(bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, Point3F* p, S32 bitCount = MATH_PRECISION)
{
   p->x = stream.readSignedFloat(bitCount);
   p->y = stream.readSignedFloat(bitCount);
   p->z = stream.readSignedFloat(bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, Point4F* p, S32 bitCount = MATH_PRECISION)
{
   p->x = stream.readSignedFloat(bitCount);
   p->y = stream.readSignedFloat(bitCount);
   p->z = stream.readSignedFloat(bitCount);
   p->w = stream.readSignedFloat(bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, Point3D* p, S32 bitCount = MATH_PRECISION)
{
   p->x = stream.readSignedFloat(bitCount);
   p->y = stream.readSignedFloat(bitCount);
   p->z = stream.readSignedFloat(bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, PlaneF* p, S32 bitCount = MATH_PRECISION)
{
   p->x = stream.readSignedFloat(bitCount);
   p->y = stream.readSignedFloat(bitCount);
   p->z = stream.readSignedFloat(bitCount);
   p->d = stream.readSignedFloat(bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, Box3F* b, S32 bitCount = MATH_PRECISION)
{
   mathRead(stream, &b->minExtents, bitCount);
   mathRead(stream, &b->maxExtents, bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, SphereF* s, S32 bitCount = MATH_PRECISION)
{
   mathRead(stream, &s->center, bitCount);
   s->radius = stream.readSignedFloat(bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, RectI* r, S32 bitCount = MATH_PRECISION)
{
   mathRead(stream, &r->point, bitCount);
   mathRead(stream, &r->extent, bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, RectF* r, S32 bitCount = MATH_PRECISION)
{
   mathRead(stream, &r->point, bitCount);
   mathRead(stream, &r->extent, bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, MatrixF* m, S32 bitCount = MATH_PRECISION)
{
   F32* pm    = *m;
   for (U32 i = 0; i < 16; i++)
      pm[i] = stream.readSignedFloat(bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, QuatF* q, S32 bitCount = MATH_PRECISION)
{
   q->x = stream.readSignedFloat(bitCount);
   q->y = stream.readSignedFloat(bitCount);
   q->z = stream.readSignedFloat(bitCount);
   q->w = stream.readSignedFloat(bitCount);
   return true;
}

inline bool mathRead(BitStream& stream, EaseF* e, S32 bitCount = MATH_PRECISION)
{
   e->dir = stream.readSignedInt(bitCount);
   e->type = stream.readSignedInt(bitCount);
   e->param[0] = stream.readSignedFloat(bitCount);
   e->param[1] = stream.readSignedFloat(bitCount);
   return true;
}

//------------------------------------------------------------------------------
//-------------------------------------- WRITING
//
inline bool mathWrite(BitStream& stream, const Point2I& p, S32 bitCount = MATH_PRECISION)
{
   stream.writeSignedInt(p.x, bitCount);
   stream.writeSignedInt(p.y, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const Point3I& p, S32 bitCount = MATH_PRECISION)
{
   stream.writeSignedInt(p.x, bitCount);
   stream.writeSignedInt(p.y, bitCount);
   stream.writeSignedInt(p.z, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const Point2F& p, S32 bitCount = MATH_PRECISION)
{
   stream.writeSignedFloat(p.x, bitCount);
   stream.writeSignedFloat(p.y, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const Point3F& p, S32 bitCount = MATH_PRECISION)
{
   stream.writeSignedFloat(p.x, bitCount);
   stream.writeSignedFloat(p.y, bitCount);
   stream.writeSignedFloat(p.z, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const Point4F& p, S32 bitCount = MATH_PRECISION)
{
   stream.writeSignedFloat(p.x, bitCount);
   stream.writeSignedFloat(p.y, bitCount);
   stream.writeSignedFloat(p.z, bitCount);
   stream.writeSignedFloat(p.w, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const Point3D& p, S32 bitCount = MATH_PRECISION)
{
   stream.writeSignedFloat(p.x, bitCount);
   stream.writeSignedFloat(p.y, bitCount);
   stream.writeSignedFloat(p.z, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const PlaneF& p, S32 bitCount = MATH_PRECISION)
{
   stream.writeSignedFloat(p.x, bitCount);
   stream.writeSignedFloat(p.y, bitCount);
   stream.writeSignedFloat(p.z, bitCount);
   stream.writeSignedFloat(p.d, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const Box3F& b, S32 bitCount = MATH_PRECISION)
{
   mathWrite(stream, b.minExtents, bitCount);
   mathWrite(stream, b.maxExtents, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const SphereF& s, S32 bitCount = MATH_PRECISION)
{
   mathWrite(stream, s.center, bitCount);
   stream.writeSignedFloat(s.radius, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const RectI& r, S32 bitCount = MATH_PRECISION)
{
   mathWrite(stream, r.point, bitCount);
   mathWrite(stream, r.extent, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const RectF& r, S32 bitCount = MATH_PRECISION)
{
   mathWrite(stream, r.point, bitCount);
   mathWrite(stream, r.extent, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const MatrixF& m, S32 bitCount = MATH_PRECISION)
{
   bool success    = true;
   const F32* pm = m;
   for (U32 i = 0; i < 16; i++)
      stream.writeSignedFloat(pm[i], bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const QuatF& q, S32 bitCount = MATH_PRECISION)
{
   stream.writeSignedFloat(q.x, bitCount);
   stream.writeSignedFloat(q.y, bitCount);
   stream.writeSignedFloat(q.z, bitCount);
   stream.writeSignedFloat(q.w, bitCount);
   return true;
}

inline bool mathWrite(BitStream& stream, const EaseF& e, S32 bitCount = MATH_PRECISION)
{
   stream.writeSignedInt(e.dir, bitCount);
   stream.writeSignedInt(e.type, bitCount);
   stream.writeSignedFloat(e.param[0], bitCount);
   stream.writeSignedFloat(e.param[1], bitCount);
   return true;
}

#endif //_MATHIO_H_

