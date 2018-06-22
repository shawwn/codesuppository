#ifndef KEY_VALUE_SPEC_H

#define KEY_VALUE_SPEC_H

#include "UserMemAlloc.h"
#include "NxFoundation.h"

// This snippets is used to convert key/value pairs rapidly to their binary versions based on a pre-defined specification.
// Basically it is used to pass a series of values efficiently as an ASCII string.
//
// Example usage:
//
// Here is the format for a specification:  <key>=<data_type>,<default_value>,<min>,<max>
//
// step_value      = NxF32,0.05,0.01,0.4       # Distance between smoothed data points
// tightness       = NxF32,1.0,0.1,100         # Distance between control points
// check_above     = NxF32,4.0,0.01,100        # Height above to start grounding
// walk_slope      = NxF32,45,1,85             # Walk slope for the character
// char_width      = NxF32,0.03,0.01,1         # The character width
// char_height     = NxF32,0.165,0.01,1        # The character height
// collision_group = string,CGF_PATH_COLLISION #
// ----------------------------------------------------------
// Example HSL usage:
// ----------------------------------------------------------
// createKeyValueSpec("GroundedPath");
// addKeyValueSpec("GroundedPath","step_value      = NxF32,0.05,0.01,0.4       # Distance between smoothed data points");
// addKeyValueSpec("GroundedPath","tightness       = NxF32,1.0,0.1,100         # Distance between control points");
// addKeyValueSpec("GroundedPath","check_above     = NxF32,4.0,0.01,100        # Height above to start grounding");
// addKeyValueSpec("GroundedPath","walk_slope      = NxF32,45,1,85             # Walk slope for the character");
// addKeyValueSpec("GroundedPath","char_width      = NxF32,0.03,0.01,1         # The character width");
// addKeyValueSpec("GroundedPath","char_height     = NxF32,0.165,0.01,1        # The character height");
// addKeyValueSpec("GroundedPath","collision_group = string,CGF_PATH_COLLISION # Specifies the collision group flags to use.");
// releaseKeyValueSpec("GroundedPath");
//
//
// v as Integer;
// v = createKeyValueData("GroundedPath","step_value=0.05,tightness=1,check_above=4,walk_slope=45,char_width=0.03,char_height=0.165,collision_group=CGF_PATH_COLLISION)
// -----------------------------------
// setKeyValueData(v,"step_value=0.06");
// -----------------------------------
// getKeyValueDataBool
// getKeyValueDataFloat
// getKeyValueDataInt
// getKeyValueDataString
// -----------------------------------
// setKeyValueDataBool
// setKeyValueDataFloat
// setKeyValueDataInt
// setKeyValueDataString

enum KeyValueType
{
  KVT_BOOLEAN,
  KVT_INTEGER,
  KVT_FLOAT,
  KVT_STRING,
  KVT_VECTOR3,
  KVT_NONE
};

class KeyValueDataItem
{
public:
  const char   *mName;
  KeyValueType  mType;
  NxU32         mIndex;

  union T
  {
    bool        mState;
    NxI32         mValue;
    NxF32       mValueF;
    char *      mString;
  } t;

  union TMIN
  {
    NxI32 mMinValue;
    NxF32 mMinValueF;
  } tmin;

  union TMAX
  {
    NxI32 mMaxValue;
    NxF32 mMaxValueF;
  } tmax;

};

class KeyValueSpec
{
public:
  virtual bool         setSpec(const char *spec) = 0; // sets the specification, erasing all previous values.
  virtual bool         addSpec(const char *spec) = 0; // add another line to the specification.
  virtual NxU32        getSpecCount(void) = 0; // return the number of data items in the specification.
  virtual bool         getSpecItem(NxU32 index,KeyValueDataItem &d) = 0;
  virtual const char  *getSpecName(void) const = 0;
  virtual const char * getSpecification(void) = 0;
  virtual NxU32        getVersionNumber(void) const = 0;

  virtual void         setUserId(NxU32 id) = 0;
  virtual NxU32        getUserId(void) = 0;
  virtual void         setUserData(void *data) = 0;
  virtual void *       getUserData(void) = 0;

  virtual NxU32        getDefaultData(void) = 0; // retrieve a data Id that corresponds to the default values for this specification.

};

class KeyValueData
{
public:
  virtual const char  *getSpecName(void) const = 0; // return the name of the parent specification.
  virtual bool         setData(const char *data) = 0; // convert ASCII data into the binary results.
  virtual const char  *getData(void) = 0;             // reconstitute the spec.
  virtual NxU32        getDataCount(void) = 0; // return the number of data items in the specification.
  virtual bool         getDataItem(NxU32 index,KeyValueDataItem &d) = 0;

  virtual bool         getState(const char *key,bool &state)  = 0;
  virtual bool         getState(const char *key,NxF32 &state)  = 0;
  virtual const char * getState(const char *key)  = 0; // retrieve it as a string.
  virtual bool         getState(const char *key,NxI32 &value)  = 0;
  virtual bool         getState(const char *key,NxVec3 &value) = 0;

  virtual bool         setState(const char *key,bool state) = 0;
  virtual bool         setState(const char *key,NxF32 state) = 0;
  virtual bool         setState(const char *key,const char *value) = 0; // retrieve it as a string.
  virtual bool         setState(const char *key,NxI32 value) = 0;
  virtual bool         setState(const char *key,const NxVec3 &v) = 0;

  virtual void         setUserId(NxU32 id) = 0;
  virtual NxU32        getUserId(void) = 0;
  virtual void         setUserData(void *data) = 0;
  virtual void *       getUserData(void) = 0;

  virtual NxU32        getVersionNumber(void) const = 0;

};


class KeyValueSpecFactory
{
public:
  virtual KeyValueSpec * createKeyValueSpec(const char *specName,const char *specification) = 0;
  virtual KeyValueSpec * locateKeyValueSpec(const char *specName) = 0;
  virtual bool           releaseKeyValueSpec(const char *specName) = 0;      // release a copy of a specification
  virtual void           releaseKeyValueSpec(KeyValueSpec *spec) = 0;      // release a copy of a specification
  virtual const char **  getKeyValueSpecList(NxU32 &count) = 0; // returns a list of all of the currently defined specifications.
  virtual NxU32          createKeyValueData(const char *specName,const char *data) = 0;
  virtual KeyValueData  *locateKeyValueData(NxU32 id) = 0;
  virtual bool           releaseKeyValueData(NxU32 data) = 0;
  virtual KeyValueData ** getKeyValueDataList(NxU32 &count) = 0;
};


void                  initDefaultKeyValueSpecFactory(void);
KeyValueSpecFactory * createKeyValueSpecFactory(void);
void                  releaseKeyValueSpecFactory(KeyValueSpecFactory *k);

extern KeyValueSpecFactory *gKeyValueSpecFactory;

#endif
