#ifndef SORTED_SET_H
#define SORTED_SET_H


#include "UserMemAlloc.h"

/*!
A template class for handling sets. A set is a sorted array where
all elements are unique. The interface to a set is very similar to
the interface of an Array. Please consult the Array documentation for
usage.

A set is optimized for concatenation and intersection with other
sets and it is also a very efficient way of keeping a list of unique
elements.
*/
template <class T>
class SortedSet
{
public:
	static const NxI32 SET_DEFAULT_SIZE = 4;

	/*!
	Default Set constructor.
	Allocates enough room for SET_DEFAULT_SIZE elements.
	*/
	SortedSet() :
		mData(0), mCapacity(SET_DEFAULT_SIZE), mSize(0)
	{
		if (mCapacity > 0)
		{
			mData = reinterpret_cast<T*> (MEMALLOC_MALLOC(sizeof(T)*mCapacity));
		}
	}

	/*!
	Set constructor.
	Allocates enough room for size elements.

	\param size
	Size of set to be defined.
	*/
	explicit SortedSet(NxU32 size) :
		mData(0), mCapacity(size), mSize(0)
	{
		if (mCapacity > 0)
		{
			mData = reinterpret_cast<T*> (MEMALLOC_MALLOC(sizeof(T)*mCapacity));
		}
	}

	/*!
	Set copy constructor.

	\param s
	The Set that will be copied.
	*/
	SortedSet(const SortedSet<T> &s) :
		mData(0), mCapacity(s.mCapacity), mSize(s.mSize)
	{
		if(mCapacity > 0)
		{
			mData = reinterpret_cast<T*> (MEMALLOC_MALLOC(sizeof(T)*mCapacity));
			memcpy(mData, s.mData, mCapacity*sizeof(T));
		}
	}

	/*!
	Default destructor.
	*/
	~SortedSet()
  {
		MEMALLOC_FREE(mData);
	}

	/*!
	Set assignment operator.
	\param s
	The set that will be copied
	\return
	A reference to the assigned set.
	*/
	inline const SortedSet<T>& operator= (const SortedSet<T>& s)
	{
		if(&s == this)
			return *this;

		mSize = s.mSize;

		if(mCapacity < mSize)
		{
			mCapacity = s.mCapacity;
			MEMALLOC_FREE(mData);
			mData = reinterpret_cast<T*> (NX_ALLOC_PERSISTENT(sizeof(T)*mCapacity));
		}
		if(mSize > 0)
			memcpy(mData, s.mData, mSize*sizeof(T));

		return *this;
	}

	/*!
	Return an element from this set. Operation is O(1).
	\param i
	The index of the element that will be returned.
	\return
	Element i in the set.
	*/
	inline const T& get(NxU32 i) const
	{
		return mData[i];
	}

	/*!
	Return an element from this set. Operation is O(1).
	\param i
	The index of the element that will be returned.
	\return
	Element i in the set.
	*/
	inline T& get(NxU32 i)
	{
		return mData[i];
	}

	/*!
	Set indexing operator.
	\param i
	The index of the element that will be returned.
	\return
	The element i in the array.
	*/
	inline const T& operator[] (NxU32 i) const
	{
		return get(i);
	}

	/*!
	Set indexing operator.
	\param i
	The index of the element that will be returned.
	\return
	The element i in the array.
	*/
	inline T& operator[] (NxU32 i)
	{
		return get(i);
	}

	/*!
	Returns the sets array representation.
	\return
	The sets representation.
	*/
	inline const T* getPtr() const
	{
		return mData;
	}

	/*!
	Returns the number of entries in the set. This can, and probably will,
	differ from the array size.
	\return
	The number of of entries in the set.
	*/
	inline NxU32 getSize() const
	{
		return mSize;
	}

	/*!
	Clears the set.
	*/
	inline void clear()
	{
		mSize=0;
	}

	/*!
	Clear the set and release the memory
	*/
	inline void release()
	{
		mSize = 0;
		mCapacity = 0;
		MEMALLOC_FREE(mData);
		mData = 0;
	}


	/*!
	Return index of a specific element.

	\param a
	The element to search for.
	\return
	The index of this element, or -1 if not found.
	*/
	inline NxI32 getIndexOf(const T &a) const
	{
		NxI32 l=0;
		NxI32 r=mSize-1;
		NxI32 i;
		for(;r>=l;)
		{
			i=(l+r)/2;
			if(mData[i]>a)
			{
				r=i-1;
			}
			else
			{
				if(mData[i]==a)
				{
					return i;
				}
				l=i+1;
			}
		}
		return -1;
	}

	/*!
	Determine if this set contains a specific element.

	\param a
	The element
	\return
	True if the element is in the set.
	False if the element is not in the set.
	*/
	inline bool contains(const T &a)
	{
		return getIndexOf(a)!=-1;
	}

	/*!
	Insert a element to the set if it does not already exists within it.
	If this Set is full it will be expanded to hold the new element.

	\param a
	The element that will be added to this set.
	\param index
	Optional pointer to index where element is inserted.
	\return
	True if the element was inserted.
	False if the element was already in the set.
	*/
	inline bool insert(const T &a, NxU32* index = NULL)
	{
		NxI32 l=0;
		NxI32 r=mSize-1;
		NxI32 i;
		for(;r>=l;)
		{
			i=(l+r)/2;
			if(mData[i]>a)
			{
				r=i-1;
			}
			else
			{
				if(mData[i]==a)
				{
					if (index)
						*index=i;

					return false;
				}
				l=i+1;
			}
		}

		//move the elements [l count-1] to [l+1 count]
		if(mCapacity<=mSize)
			grow(1+mCapacity*2);

		if (mSize-l > 0)
			memmove(&mData[l+1],&mData[l],(mSize-l)*sizeof(T));

		mData[l]=a;
		mSize++;

		if (index)
			*index=l;

		return true;
	}

	/*!
	Remove one element from the set, if it exists.
	\param a
	The element that will be subtracted from this set.
	\return
	True if the element was removed. False if no 
	element was found.
	*/
	inline bool remove(const T &a) 
	{
		NxI32 l=0;
		NxI32 r=mSize-1;
		NxI32 i;
		for(;r>=l;) 
		{
			i=(l+r)/2;
			if(mData[i]>a) 
			{
				r=i-1;
			}
			else 
			{
				if(mData[i]==a) 
				{
					memmove(&mData[i],&mData[i+1],(mSize-i-1)*sizeof(T));
					mSize--;
					return true;
				}
				l=i+1;
			}
		}
		return false;
	}

	/*!
	Subtracts the element on position i from the set.
	Operation is O(n). Fastest removal on high index elements.
	\param i
	The position of the element that will be subtracted from this set.
	\return
	The element that was removed.
	*/
	inline T removeElement(NxU32 i) 
	{
		T tmp = mData[i];
		memmove(mData+i,mData+i+1,(mSize-i-1)*sizeof(T));
		mSize--;
		return tmp;
	}

private:

	/*!
	Resizes the available memory for the array representing the set.

	\param capacity
	The number of entries that the set should be able to hold.
	*/	
	inline void grow(NxU32 capacity) 
	{
		if(this->mCapacity < capacity)
    {
			T* newData = reinterpret_cast<T*>(MEMALLOC_MALLOC(capacity*sizeof(T)));
			memcpy(newData, mData, mSize*sizeof(T));

			MEMALLOC_FREE(mData);
			mData = newData;

			//initialize new entries
			for(NxU32 i = mCapacity; i < capacity; i++)
			{
				new ((void*)(mData+i)) T;
			}
			mCapacity = capacity;
		}
	}

	T *					mData;
	NxU32				mCapacity;
	NxU32				mSize;
};

#endif
