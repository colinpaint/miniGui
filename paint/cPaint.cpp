// cPaint.cpp
//{{{  includes
#include <deque>
#include "../common/basicTypes.h"
#include "../common/cLog.h"

#include "../gui/cTexture.h"
#include "cPaint.h"

using namespace std;
using namespace chrono;
//}}}

// cPaintLayer
//{{{
cPaintLayer::cPaintLayer (const std::string& name, const cColor& color, cPoint pos, float width)
    : cLayer(name, color, {0.f,0.f}), mWidth(width) {

  setRadius (width);
  addPoint (pos, 0, 0);
  }
//}}}
//{{{
void cPaintLayer::addPoint (cPoint pos, int pressure, int timestamp) {

   if (mLine.empty() || ((pos - mLine.back().mPos).magnitude() > mWidth))
     mLine.push_back (sBrushPoint (pos, pressure, timestamp));
   }
//}}}
//{{{
bool cPaintLayer::pick (cPoint pos) {
// !!! should be distance to line test !!!

  return mExtent.inside (pos);
  }
//}}}
//{{{
void cPaintLayer::prox (cPoint pos) {
  (void)pos;
  mProx = true;
  }
//}}}
//{{{
void cPaintLayer::proxExit() {
  mProx = false;
  }
//}}}
//{{{
void cPaintLayer::proxLift() {
  mProx = false;
  }
//}}}
//{{{
void cPaintLayer::down (cPoint pos) {
  (void)pos;
  mProx = false;
  }
//}}}
//{{{
void cPaintLayer::move (cPoint pos, cPoint inc, int pressure, int timestamp) {
  (void)pos;
  (void)pressure;
  (void)timestamp;

  mPos += inc;
  }
//}}}
//{{{
void cPaintLayer::up (cPoint pos, bool mouseMoved) {
  (void)pos;
  (void)mouseMoved;
  }
//}}}
//{{{
void cPaintLayer::wheel (int delta, cPoint pos) {
  (void)pos;
  mWidth = mWidth * (delta > 0 ? 1.05f : 1/1.05f);
  }
//}}}
//{{{
void cPaintLayer::draw (cWindow& window) {

  mExtent = {0,0,0,0};

  if (mLine.empty())
    return;

  bool first = true;
  for (auto& brushPoint : mLine) {
    paint (window, kYellow, mPos + brushPoint.mPos, brushPoint.mPressure, first);
    mExtent |= mPos + brushPoint.mPos;
    first = false;
    }
  }
//}}}

// private
//{{{
uint8_t cPaintLayer::getPaintShape (float i, float j, float radius, float pressure) {
  return static_cast<uint8_t>(pressure * (1.f - clamp (sqrtf((i*i) + (j*j)) - radius, 0.f, 1.f)));
  }
//}}}
//{{{
void cPaintLayer::setRadius (float radius) {

  mOverlap = 0.25f;
  mMaxPressure = 0.5f;

  mRadius = radius;
  mShapeRadius = static_cast<unsigned>(ceil(radius));
  mShapeSize = (2 * mShapeRadius) + 1;

  mSubPixels = 4;
  mSubPixelResolution = 1.f / mSubPixels;

  // subPixel array
  for (int ySubPixel = 0; ySubPixel < mSubPixels; ySubPixel++) {
    for (int xSubPixel = 0; xSubPixel < mSubPixels; xSubPixel++) {
      mBrushShapes [(ySubPixel * mSubPixels) + xSubPixel].release();
      mBrushShapes [(ySubPixel * mSubPixels) + xSubPixel].createPixels (mShapeSize, mShapeSize);
      uint8_t* shape = mBrushShapes [(ySubPixel * mSubPixels) + xSubPixel].getPixels();
      for (int j = -mShapeRadius; j <= mShapeRadius; j++)
        for (int i = -mShapeRadius; i <= mShapeRadius; i++)
          *shape++ = getPaintShape (i - (xSubPixel * mSubPixelResolution),
                                    j - (ySubPixel * mSubPixelResolution), mRadius, 255 * mMaxPressure);
      }
    }

  // single shape
  mBrushShape.release();
  mBrushShape.createPixels (mShapeSize, mShapeSize);
  }
//}}}
//{{{
void cPaintLayer::stamp (cWindow& window, const cColor& color, cPoint pos, int pressure) {
// calc and stamp brushShape into image

  // !!!! does this behave correctly as we go negative !!!
  int32_t xInt = static_cast<int32_t>(pos.x);
  float xSubPixel = pos.x - xInt;

  int32_t yInt = static_cast<int32_t>(pos.y);
  float ySubPixel = pos.y - yInt;

  // calc subPixel brush shape
  uint8_t* shape = mBrushShape.getPixels();
  for (int j = -mShapeRadius; j <= mShapeRadius; j++)
    for (int i = -mShapeRadius; i <= mShapeRadius; i++)
      *shape++ = getPaintShape (i - (xSubPixel * mSubPixelResolution),
                                j - (ySubPixel * mSubPixelResolution), mRadius,
                                (255.f * pressure * mMaxPressure) / 1024.f);

  // stamp to point
  window.stamp (color, mBrushShape, cPoint(xInt, yInt) - cPoint (mShapeRadius, mShapeRadius));

  mPrevPos = pos;
  }
//}}}
//{{{
void cPaintLayer::stampPreCalc (cWindow& window, const cColor& color, cPoint pos, int pressure) {
// stamp brushShape into image
  (void)pressure;

  // !!!! does this behave correctly as we go negative !!!
  int32_t xInt = static_cast<int32_t>(pos.x);
  float xSubPixel = pos.x - xInt;
  int32_t xSubIndex = static_cast<int>(xSubPixel / mSubPixelResolution);

  int32_t yInt = static_cast<int32_t>(pos.y);
  float ySubPixel = pos.y - yInt;
  int32_t ySubIndex = static_cast<int>(ySubPixel / mSubPixelResolution);

  // point to first clipped shape pix
  window.stamp (color, mBrushShapes [(ySubIndex * mSubPixels) + xSubIndex],
                cPoint(xInt, yInt) - cPoint (mShapeRadius, mShapeRadius));

  mPrevPos = pos;
  }
//}}}
//{{{
void cPaintLayer::paint (cWindow& window, const cColor& color, cPoint pos, int pressure, bool first) {

  cColor color1 = color;
  color1.a = 0.5f;

  if (first)
    stamp (window, color1, pos, pressure);
  else {
    // draw stamps from mPrevPos to pos
    cPoint diff = pos - mPrevPos;
    float length = diff.magnitude();
    float overlap = mRadius * mOverlap;

    if (length >= overlap) {
      cPoint inc = diff * (overlap / length);

      unsigned numStamps = static_cast<unsigned>(length / overlap);
      for (unsigned i = 0; i < numStamps; i++)
        stamp (window, color1, mPrevPos + inc, pressure);
      }
    }
  }
//}}}

//{{{  cStrokeLayer
//{{{
cStrokeLayer::cStrokeLayer (const std::string& name, const cColor& color, cPoint pos, float width)
    : cLayer(name, color, {0.f,0.f}), mWidth(width) {
  addPoint (pos, 0,0);
  }
//}}}

//{{{
void cStrokeLayer::addPoint (cPoint pos, int pressure, int timestamp) {

   if (mLine.empty() || ((pos - mLine.back().mPos).magnitude() > mWidth))
     mLine.push_back (sBrushPoint (pos, pressure, timestamp));
   }
//}}}

//{{{
bool cStrokeLayer::pick (cPoint pos) {
// !!! should be distance to line test !!!

  return mExtent.inside (pos);
  }
//}}}

//{{{
void cStrokeLayer::prox (cPoint pos) {
  (void)pos;
  mProx = true;
  }
//}}}
//{{{
void cStrokeLayer::proxExit() {
  mProx = false;
  }
//}}}
//{{{
void cStrokeLayer::proxLift() {
  mProx = false;
  }
//}}}

//{{{
void cStrokeLayer::down (cPoint pos) {
  (void)pos;
  mProx = false;
  }
//}}}
//{{{
void cStrokeLayer::move (cPoint pos, cPoint inc, int pressure, int timestamp) {
  (void)pos;
  (void)pressure;
  (void)timestamp;

  mPos += inc;
  }
//}}}
//{{{
void cStrokeLayer::up (cPoint pos, bool mouseMoved) {
  (void)pos;
  (void)mouseMoved;
  }
//}}}

//{{{
void cStrokeLayer::wheel (int delta, cPoint pos) {
  (void)pos;
  mWidth = mWidth * (delta > 0 ? 1.05f : 1/1.05f);
  }
//}}}

//{{{
void cStrokeLayer::draw (cWindow& window) {

  mExtent = {0,0,0,0};

  if (mLine.empty())
    return;

  cPoint lastPos;
  bool first = true;
  for (auto& brushPoint : mLine) {
    if (first)
      first = false;
    else
      window.drawLine (mProx ? kLightBlue : mColor, mPos + lastPos, mPos + brushPoint.mPos, mWidth);
    lastPos = brushPoint.mPos;

    mExtent |= mPos + brushPoint.mPos;
    }

  first = true;
  for (auto& brushPoint : mLine) {
    if (first) {
      first = false;
      }
    else {
      cPoint perp = (brushPoint.mPos - lastPos).perp() * (16.f * brushPoint.mPressure / 1024.f);
      window.drawLine (kWhite, mPos + brushPoint.mPos - perp, mPos + brushPoint.mPos + perp, 1.f);
      }
    lastPos = brushPoint.mPos;
    }
  }
//}}}
//}}}
//{{{  cRectangleLayer
//{{{
bool cRectangleLayer::pick (cPoint pos) {
  cRect r (mPos, mPos + mLength);
  return r.inside (pos);
  }
//}}}

//{{{
void cRectangleLayer::prox (cPoint pos) {
  (void)pos;
  }
//}}}
//{{{
void cRectangleLayer::proxExit() {
  }
//}}}
//{{{
void cRectangleLayer::proxLift() {
  mProx = false;
  }
//}}}

//{{{
void cRectangleLayer::down (cPoint pos) {
  (void)pos;
  }
//}}}
//{{{
void cRectangleLayer::move (cPoint pos, cPoint inc, int pressure, int timestamp) {
  (void)pos;
  (void)pressure;
  (void)timestamp;

  mPos += inc;
  }
//}}}
//{{{
void cRectangleLayer::up (cPoint pos, bool mouseMoved) {
  (void)pos;
  (void)mouseMoved;
  }
//}}}

//{{{
void cRectangleLayer::wheel (int delta, cPoint pos) {
  (void)pos;
  mLength = mLength * (delta > 0 ? 1.05f : 1/1.05f);
  }
//}}}

//{{{
void cRectangleLayer::draw (cWindow& window) {
  window.drawRectangle (mColor, {mPos, mPos + mLength});
  }
//}}}
//}}}
//{{{  cEllipseLayer
//{{{
bool cEllipseLayer::pick (cPoint pos) {
  return (mPos - pos).magnitude() < mRadius;
  }
//}}}

//{{{
void cEllipseLayer::prox (cPoint pos) {
  (void)pos;
  }
//}}}
//{{{
void cEllipseLayer::proxExit() {
  }
//}}}
//{{{
void cEllipseLayer::proxLift() {
  mProx = false;
  }
//}}}

//{{{
void cEllipseLayer::down (cPoint pos) {
  (void)pos;
  }
//}}}
//{{{
void cEllipseLayer::move (cPoint pos, cPoint inc, int pressure, int timestamp) {
  (void)pos;
  (void)pressure;
  (void)timestamp;

  mPos += inc;
  }
//}}}
//{{{
void cEllipseLayer::up (cPoint pos, bool mouseMoved) {
  (void)pos;
  (void)mouseMoved;
  }
//}}}

//{{{
void cEllipseLayer::wheel (int delta, cPoint pos) {
  (void)pos;
  mRadius += delta * 1.05f;
  }
//}}}

//{{{
void cEllipseLayer::draw (cWindow& window) {
  window.drawEllipse (mColor, mPos, mRadius, mWidth);
  }
//}}}
//}}}
//{{{  cTextLayer
//{{{
bool cTextLayer::pick (cPoint pos) {
  cRect r(mPos, mPos + mLength);
  return r.inside(pos);
}
//}}}

//{{{
void cTextLayer::prox (cPoint pos) {
  (void)pos;
  }
  //}}}
//{{{
void cTextLayer::proxExit() {
  }
//}}}
//{{{
void cTextLayer::proxLift() {
  mProx = false;
  }
//}}}

//{{{
void cTextLayer::down (cPoint pos) {
  (void)pos;
  }
//}}}
//{{{
void cTextLayer::move (cPoint pos, cPoint inc, int pressure, int timestamp) {
  (void)pos;
  (void)pressure;
  (void)timestamp;

  mPos += inc;
  }
//}}}
//{{{
void cTextLayer::up (cPoint pos, bool mouseMoved) {
  (void)pos;
  (void)mouseMoved;
  }
//}}}

//{{{
void cTextLayer::wheel (int delta, cPoint pos) {
  (void)delta;
  (void)pos;
  }
//}}}

//{{{
void cTextLayer::draw (cWindow& window) {
  mLength = window.drawText (mColor, cRect (mPos, window.getSize()), mText);
  }
//}}}
//}}}
//{{{  cTextureLayer
//{{{
bool cTextureLayer::pick (cPoint pos) {
  return mExtent.inside (pos);
  }
//}}}

//{{{
void cTextureLayer::prox (cPoint pos) {
  (void)pos;
  }
//}}}
//{{{
void cTextureLayer::proxExit() {
  }
//}}}
//{{{
void cTextureLayer::proxLift() {
  mProx = false;
  }
//}}}

//{{{
void cTextureLayer::down (cPoint pos) {
  (void)pos;
  }
//}}}
//{{{
void cTextureLayer::move (cPoint pos, cPoint inc, int pressure, int timestamp) {
  (void)pos;
  (void)pressure;
  (void)timestamp;

  mPos += inc;
  }
//}}}
//{{{
void cTextureLayer::up (cPoint pos, bool mouseMoved) {
  (void)pos;
  (void)mouseMoved;
  }
//}}}

//{{{
void cTextureLayer::wheel (int delta, cPoint pos) {
  (void)pos;
  mSize *= delta > 0 ? 1.05f : 1/1.05f;
  }
//}}}

//{{{
void cTextureLayer::draw (cWindow& window) {

  window.blitAffine (mTexture, window.getSize(), mSize, mAngle, mPos.x, mPos.y);
  mExtent = {mPos - ((mTexture.getSize() / 2.f) * mSize), mPos + ((mTexture.getSize() /2.f) * mSize)};
  }
//}}}
//}}}

// cPaint
//{{{
cPaint::cPaint (cWindow& window) : mWindow(window) {
  }
//}}}
//{{{
 cPaint::~cPaint() {
  // deallocate layers
  }
//}}}
//{{{
cLayer* cPaint::addLayer (cLayer* layer) {
  mLayers.push_back (layer);
  return layer;
  }
//}}}
//{{{
bool cPaint::pick (cPoint pos) {

  for (auto layerIt = mLayers.rbegin(); layerIt != mLayers.rend(); ++layerIt) {
    if ((*layerIt)->pick (pos)) {
      mPickedLayer = *layerIt;
      return true;
      }
    }

  mPickedLayer = nullptr;
  return false;
  }
//}}}
//{{{
bool cPaint::prox (cPoint pos) {
  if (mPickedLayer)
    mPickedLayer->prox (pos);
  return true;
  }
//}}}
//{{{
bool cPaint::proxExit() {
  if (mPickedLayer)
    mPickedLayer->proxExit();
  return true;
  }
//}}}
//{{{
bool cPaint::proxLift() {
  if (mPickedLayer)
    mPickedLayer->proxLift();
  return true;
  }
//}}}
//{{{
bool cPaint::down (cPoint pos) {

  if (mPainting)
    mPickedLayer = addLayer (new cPaintLayer ("paint", kYellow, pos, 16.f));
  else if (mStroking)
    mPickedLayer = addLayer (new cStrokeLayer ("stroke", kGreen, pos, 4.f));
  else if (mPickedLayer)
    mPickedLayer->down (pos);

  return true;
  }
//}}}
//{{{
bool cPaint::move (cPoint pos, cPoint inc, int pressure, int timestamp) {

  if (mPainting || mStroking)
    mPickedLayer->addPoint (pos, pressure, timestamp);
  else if (mPickedLayer)
    mPickedLayer->move (pos, inc, pressure, timestamp);

  return true;
  }
//}}}
//{{{
bool cPaint::up (cPoint pos, bool mouseMoved){

  if (mPainting || mStroking) {
    }
  else if (mPickedLayer)
    mPickedLayer->up (pos, mouseMoved);

  return true;
  }
//}}}
//{{{
bool cPaint::wheel (int delta, cPoint pos) {

  if (mPickedLayer)
    mPickedLayer->wheel (delta, pos);

  return true;
  }
//}}}
//{{{
void cPaint::draw() {
  for (auto layer : mLayers)
    layer->draw (mWindow);
  }
//}}}
