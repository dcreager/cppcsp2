/*
*	The Kent C++CSP Library 
*	Copyright (C) 2002-2007 Neil Brown
*
*	This library is free software; you can redistribute it and/or
*	modify it under the terms of the GNU Lesser General Public
*	License as published by the Free Software Foundation; either
*	version 2.1 of the License, or (at your option) any later version.
*
*	This library is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*	Lesser General Public License for more details.
*
*	You should have received a copy of the GNU Lesser General Public
*	License along with this library; if not, write to the Free Software
*	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

namespace csp
{	
	/**
	*	Templated class implementing MOBILE semantics.
	*
	*	A mobile holds a pointer that ensures (as much as possible in C++) that only one mobile can hold that pointer at any one time.
	*	A pointer can be transferred between two mobiles using the copy constructor or assignment.  This is a transfer,
	*	not a copy; the source of the transfer becomes blank after the operation (its pointer is set to NULL).	
	*	If you assign to a mobile that is already holding a non-NULL pointer, the object is deleted, and then the transfer
	*	takes place as described.  If the mobile holds a non-NULL pointer when it is destroyed, the object will be deleted.	
	*
	*	Communicating mobiles via channels will maintain these semantics (as channels use an object's assignment operator) 
	*	of mobiles and hence mobiles provide a simple means of safely passing (pointers to) large objects through channels,
	*	while maintaining the CREW principle (by making it EREW! Exclusive Read and Exclusive Write).
	*
		The behaviour of csp::Mobile is very similar to <a href="http://www.gotw.ca/publications/using_auto_ptr_effectively.htm">std::auto_ptr</a>.
	*	There is one important difference, however, which allows csp::Mobile to be communicated over channels, whereas
	*	std::auto_ptr could not.  csp::Mobile's assignment operator takes a const csp::Mobile as its argument, whereas 
	*	std::auto_ptr takes a [non-const] std::auto_ptr.  Because the items written to a channel must be const, std::auto_ptr
	*	could not be sent over a channel, but csp::Mobile can.  csp::Mobile uses the mutable keyword to break the const semantics --
	*	it's not pretty, but it's effective.	
	*
	*	@ref tut-mobile "Mobiles" are covered in the guide.
	*
	*	@section tempreq DATA_TYPE Requirements
	*
	*	DATA_TYPE* must be a valid type; in particular, DATA_TYPE must not be a reference.  If you want to use the clone()
	*	method, DATA_TYPE must have a public copy constructor.  If you want to use a comparison operator for Mobile<DATA_TYPE>,
	*	that same operator must be defined for DATA_TYPE.
	*/
	template <class DATA_TYPE>
	class Mobile
	{
	private:
		/**
		*	The actual pointer.  Absolutely <em>MUST</em> be mutable
		*/
		mutable DATA_TYPE* data;

	public:
		/**
		*	Constructs a mobile with the given data.  The data should be on the heap (allocated using new), and should not be passed to two mobiles.
		*	The data will be deleted by this class, hence it must be allocated using new.  Because it is deleted by this class,
		*	it should never be used by two mobiles (they would both try to delete it) and for good practice should be passed to
		*	the mobile at its point of allocation, and used via the mobile forever after.		
		*
		*	@param _data The data for the mobile to point to
		*
		*	The problem with mobiles in C++ (as opposed to occam) is that
		*	you could pass the same pointer to the constructors of 2 mobiles
		*	and hence break CREW.  But there is nothing
		*	we can do about that, so we will have to rely on the user (you) to make sure this does not happen.
		*/
		inline explicit Mobile(DATA_TYPE* const _data = NULL)
			:	data(_data)
		{			
		}

		/**
		*	Copy constructor.  Takes the pointer of the passed mobile, and then blanks the source mobile.
		*
		*	@post The data pointer in mob will always be NULL after this operation, even though mob is const.
		*
		*	@param mob The mobile to assign from
		*/
		inline Mobile(const Mobile<DATA_TYPE>& mob)
		{			
			data = mob.data;
			mob.data = NULL;
		}

		/**
		*	Destructor.  Calls blank().  @see blank
		*/
		inline ~Mobile()
		{
			blank();
		}

		/**
		*	Blanks the mobile by deleting the data and setting pointer to NULL.
		*
		*	@post The data pointer in this mobile will always be NULL after this operation.
		*/
		inline void blank()
		{
			if (data != NULL)
				delete data;
			data = NULL;
		};

		/**
		*	Easy and neat way of checking for the mobile being empty
		*
		*	A mobile "is" true if its pointer is not NULL
		*
		*	@return Whether the pointer is non-NULL
		*/
		inline operator bool () const
		{
			return data != NULL;
		};

		/**
		*	Mobile assignment operator.
		*	
		*	This assignment takes the data pointer of the source mobile into this mobile, and then blanks the source mobile.
		*		
		*	If this mobile already has some data in it, the data will be deleted during the assignment.
		*		
		*	@post The data pointer in mob will always be NULL after this operation, even though mob is const.		
		*/
		inline void operator=(const Mobile<DATA_TYPE>& mob)		
		{
			if (data !=	NULL)
				delete data;
			data = mob.data;
			mob.data = NULL;		
		};

		/**
		*	Allows this class to be used in place of a pointer.		
		*/
		inline DATA_TYPE* operator ->() const {return data;}
		
		/**
		*	Allows this class to be used in place of a pointer.		
		*/		
		inline DATA_TYPE& operator*() const {return *data;}
		
		/**
		*	Provides a way to get the data pointer inside the mobile.
		*
		*	This method should be used sparingly if at all.  Usually, the -> operator of mobile
		*	will be sufficient (and an easier way) to use the data inside the mobile.  However,
		*	sometimes you may need the pointer to the object inside the class, for example to
		*	pass to a function expected DATA_TYPE* rather than Mobile<DATA_TYPE>.  You should
		*	not use this method as a way to pass the pointer around - the object inside will
		*	get deleted if this object is assigned to, so the pointer may not be valid for
		*	as long as you hope.
		*/
		inline DATA_TYPE* get() const {return data;}
		
		/**
		*	Clones the mobile.
		*
		*	This function will cause an error upon compilation unless the class DATA_TYPE
		*	is either: primitive, uses a default copy constructor, or has
		*	a copy constructor that is publicly accessible.
		*	The copy constructor defines the behaviour of the clone;
		*	the new Mobile will be a separate object, pointing to a new object
		*	copy constructed from the current data.
		*
		*	If the Mobile is blank (NULL), then a new blank Mobile will
		*	be returned, without error.
		*
		*	This mobile will not be altered by this operation.		
		*/
		inline Mobile<DATA_TYPE> clone() const
		{
			if (data == NULL)
				return Mobile<DATA_TYPE>(NULL);
			else
				return Mobile<DATA_TYPE>(new DATA_TYPE(*data));
		};		
		
		/** 
		*	Tests whether the objects inside two mobiles are equal.
		*
		*	A first intuition about the equality of mobiles might be that it should be based on whether the pointers
		*	inside the mobiles are equal.  However, the pointers should always be different, so if they were equal it
		*	would mean you are going to crash your application!
		*
		*	Therefore comparison of mobiles is based on a comparison of the inner types.  If you wish to use
		*	comparison operators of Mobile<DATA_TYPE>, make sure those same operators are defined for DATA_TYPE.		
		*
		*	If DATA_TYPE inherits from boost::noncopyable (or for other reasons does not have a public copy constructor)
		*	be sure to use references when implementing DATA_TYPE::operator==, to avoid the compiler trying to copy
		*	DATA_TYPE for the function call
		*/
		inline bool operator==(const Mobile<DATA_TYPE>& mob) const
		{
			return *data == *(mob.data);
		}
		
		/**
		*	See the documentation for operator==
		*/		
		inline bool operator<(const Mobile<DATA_TYPE>& mob) const
		{
			return *data < *(mob.data);
		}				

	}; //class Mobile<DATA_TYPE>

} //namespace csp



