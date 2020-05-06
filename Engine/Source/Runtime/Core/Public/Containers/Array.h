#pragma once

#include "CoreTypes.h"

#include "HAL/Allocators/AnsiAllocator.h"

#include "Templates/Forward.h"
#include "Templates/EnableIf.h"
#include "Templates/TypeTraits.h"
#include "Templates/MoveTemp.h"
#include "Templates/IsBitwiseConstructible.h"
#include "Templates/IsTriviallyDestructable.h"
#include "Templates/IsBitwiseRelocatable.h"

namespace Nexus
{

	namespace Private
	{

		template <typename FFromArrayType, typename FToArrayType>
		struct TCanMoveTArrayPointersBetweenArrayTypes
		{
			using FFromAllocatorType = typename FFromArrayType::FAllocator;
			using FToAllocatorType = typename FToArrayType::FAllocator;
			using FFromElementType = typename FFromArrayType::FElementType;
			using FToElementType = typename FToArrayType::FElementType;

			enum
			{
				Value =
					TAreTypesEqual<FFromAllocatorType, FToAllocatorType>::Value && // Allocators must be equal
					(
						TAreTypesEqual         <FToElementType, FFromElementType>::Value || // The element type of the container must be the same, or...
						TIsBitwiseConstructible<FToElementType, FFromElementType>::Value    // ... the element type of the source container must be bitwise constructible from the element type in the destination container
					)
			};
		};

	}

	template<typename FElementType, typename FAllocator = FAnsiAllocator>
	class TArray
	{

	private:

		template <typename FOtherElementType, typename FOtherAllocator>
		friend class TArray;

		using FSizeType = typename FAllocator::FSizeType;

	public:

		// Constructors.

		/**
		 * Constructor, initializes element number counters.
		 */
		FORCEINLINE TArray(void)
			: ArrayNum(0)
			, ArrayMax(0)
		{
		}

		/**
		 * Constructor from a raw array of elements.
		 *
		 * @param Ptr   A pointer to an array of elements to copy.
		 * @param Count The number of elements to copy from Ptr.
		 */
		FORCEINLINE TArray(const FElementType* Ptr, FSizeType Count)
		{
			Check(Ptr != nullptr || Count == 0);
			CopyToEmpty(Ptr, Count, 0, 0);
		}

		/**
		 * Initializer list constructor.
		 */
		TArray(const std::initializer_list<FElementType>& InitList)
		{
			CopyToEmpty(InitList.begin(), static_cast<FSizeType>(InitList.size()), 0, 0);
		}

		/**
		 * Copy constructor. Use the common routine to perform the copy.
		 *
		 * @param Other The source array to copy.
		 */
		FORCEINLINE TArray(const TArray& Other)
		{
			CopyToEmpty(Other.GetData(), Other.Num(), 0, 0);
		}

		/**
		 * Copy constructor with changed allocator. Use the common routine to perform the copy.
		 *
		 * @param Other The source array to copy.
		 */
		template <typename FOtherElementType, typename FOtherAllocator>
		FORCEINLINE explicit TArray(const TArray<FOtherElementType, FOtherAllocator>& Other)
		{
			CopyToEmpty(Other.GetData(), Other.Num(), 0, 0);
		}

		/**
		 * Move constructor.
		 *
		 * @param Other Array to move from.
		 */
		FORCEINLINE TArray(TArray&& Other)
		{
			MoveOrCopy(*this, Other, 0);
		}

		/**
		 * Move constructor.
		 *
		 * @param Other Array to move from.
		 */
		template <typename OtherElementType, typename OtherAllocator>
		FORCEINLINE explicit TArray(TArray<OtherElementType, OtherAllocator>&& Other)
		{
			MoveOrCopy(*this, Other, 0);
		}

		/** 
		 * Destructor. 
		 */
		inline ~TArray()
		{
			DestructItems(GetData(), ArrayNum);
		}

	public:

		// Operators.

		/**
		 * Initializer list assignment operator. First deletes all currently contained elements
		 * and then copies from initializer list.
		 *
		 * @param InitList The initializer list to copy from.
		 */
		TArray& operator=(const std::initializer_list<FElementType>& InitList)
		{
			DestructItems(GetData(), ArrayNum);
			CopyToEmpty(InitList.begin(), static_cast<FSizeType>(InitList.size()), ArrayMax, 0);

			return *this;
		}

		/**
		 * Assignment operator. First deletes all currently contained elements
		 * and then copies from other array.
		 *
		 * @param Other The source array to assign from.
		 */
		TArray& operator=(const TArray& Other)
		{
			if (this != &Other)
			{
				DestructItems(GetData(), ArrayNum);
				CopyToEmpty(Other.GetData(), Other.Num(), ArrayMax, 0);
			}

			return *this;
		}

		/**
		 * Assignment operator. First deletes all currently contained elements
		 * and then copies from other array.
		 *
		 * Allocator changing version.
		 *
		 * @param Other The source array to assign from.
		 */
		template<typename OtherAllocator>
		TArray& operator=(const TArray<FElementType, OtherAllocator>& Other)
		{
			DestructItems(GetData(), ArrayNum);
			CopyToEmpty(Other.GetData(), Other.Num(), ArrayMax, 0);

			return *this;
		}

		/**
		 * Move assignment operator.
		 *
		 * @param Other Array to assign and move from.
		 */
		TArray& operator=(TArray&& Other)
		{
			if (this != &Other)
			{
				DestructItems(GetData(), ArrayNum);
				MoveOrCopy(*this, Other, ArrayMax);
			}

			return *this;
		}

	public:

		// Index operators.

		/**
		 * Array bracket operator. Returns reference to element at give index.
		 *
		 * @returns Reference to indexed element.
		 */
		FORCEINLINE FElementType& operator[](FSizeType Index)
		{
			RangeCheck(Index);
			return GetData()[Index];
		}

		/**
		 * Array bracket operator. Returns reference to element at give index.
		 *
		 * Const version of the above.
		 *
		 * @returns Reference to indexed element.
		 */
		FORCEINLINE const FElementType& operator[](FSizeType Index) const
		{
			RangeCheck(Index);
			return GetData()[Index];
		}

	public:

		// Comparison operators.

		/**
		 * Equality operator.
		 *
		 * @param OtherArray Array to compare.
		 * @returns True if this array is the same as OtherArray. False otherwise.
		 */
		bool operator==(const TArray& OtherArray) const
		{
			return Num() == OtherArray.Num() 
				&& CompareItems(GetData(), OtherArray.GetData(), Count);
		}

		/**
		 * Inequality operator.
		 *
		 * @param OtherArray Array to compare.
		 * @returns True if this array is NOT the same as OtherArray. False otherwise.
		 */
		FORCEINLINE bool operator!=(const TArray& OtherArray) const
		{
			return !(*this == OtherArray);
		}

	public:

		// Content modifier functions.

		/**
		 * Adds a new item to the end of the array, possibly reallocating the whole array to fit.
		 *
		 * @param Item The item to add
		 * @return Index to the new item
		 * @see AddDefaulted, AddUnique, AddZeroed, Append, Insert
		 */
		FORCEINLINE FSizeType Add(const FElementType& Item)
		{
			CheckAddress(&Item);
			return Emplace(Item);
		}

		/**
		 * Adds a new item to the end of the array, possibly reallocating the whole array to fit.
		 *
		 * Move semantics version.
		 *
		 * @param Item The item to add
		 * @return Index to the new item
		 */
		FORCEINLINE FSizeType Add(FElementType&& Item)
		{
			CheckAddress(&Item);
			return Emplace(MoveTempIfPossible(Item));
		}

		/**
		 * Inserts a given element into the array at given location.
		 *
		 * @param Item The element to insert.
		 * @param Index Tells where to insert the new elements.
		 *
		 * @returns Location at which the insert was done.
		 */
		FSizeType Insert(const FElementType& Item, FSizeType Index)
		{
			CheckAddress(&Item);

			InsertUninitialized(Index, 1);
			new(GetData() + Index) FElementType(Item);

			return Index;
		}

		/**
		 * Inserts a given element into the array at given location. Move semantics
		 * version.
		 *
		 * @param Item The element to insert.
		 * @param Index Tells where to insert the new elements.
		 * @returns Location at which the insert was done.
		 * @see Add, Remove
		 */
		FSizeType Insert(FElementType&& Item, FSizeType Index)
		{
			CheckAddress(&Item);

			InsertUninitialized(Index, 1);
			new(GetData() + Index) FElementType(MoveTempIfPossible(Item));

			return Index;
		}

		/**
		 * Removes an element (or elements) at given location optionally shrinking
		 * the array.
		 *
		 * @param Index Location in array of the element to remove.
		 * @param Count (Optional) Number of elements to remove. Default is 1.
		 * @param bAllowShrinking (Optional) Tells if this call can shrink array if suitable after remove. Default is true.
		 */
		FORCEINLINE void RemoveAt(FSizeType Index)
		{
			RemoveAtImpl(Index, 1, true);
		}

		/**
		 * Constructs a new item at the end of the array, possibly reallocating the whole array to fit.
		 *
		 * @param Args	The arguments to forward to the constructor of the new item.
		 * @return		Index to the new item
		 */
		template <typename... FArgsType>
		FORCEINLINE FSizeType Emplace(FArgsType&&... Args)
		{
			const FSizeType Index = AddUninitialized(1);

			new(GetData() + Index) FElementType(TForward<FArgsType>(Args)...);
			return Index;
		}

	public:

		// Filter functions.

		/**
		 * Checks if this array contains the element.
		 *
		 * @returns	True if found. False otherwise.
		 * @see ContainsByPredicate, FilterByPredicate, FindByPredicate
		 */
		template <typename FComparisonType>
		bool Contains(const FComparisonType& Item) const
		{
			for (const FElementType* RESTRICT Data = GetData(), *RESTRICT DataEnd = Data + ArrayNum; Data != DataEnd; ++Data)
			{
				if (*Data == Item)
				{
					return true;
				}
			}
			return false;
		}

	public:

		// Simple functions.

		/**
		 * Helper function for returning a typed pointer to the first array entry.
		 *
		 * @returns Pointer to first array entry or nullptr if ArrayMax == 0.
		 */
		FORCEINLINE FElementType* GetData()
		{
			return static_cast<FElementType*>(AllocatorInstance.GetAllocation());
		}

		/**
		 * Helper function for returning a typed pointer to the first array entry.
		 *
		 * @returns Pointer to first array entry or nullptr if ArrayMax == 0.
		 */
		FORCEINLINE const FElementType* GetData() const
		{
			return (const FElementType*)AllocatorInstance.GetAllocation();
		}

		/**
		 * Returns number of elements in array.
		 *
		 * @returns Number of elements in array.
		 * @see GetSlack
		 */
		FORCEINLINE FSizeType Num() const
		{
			return ArrayNum;
		}

		/**
		 * Helper function returning the size of the inner type.
		 *
		 * @returns Size in bytes of array type.
		 */
		FORCEINLINE uint32 GetTypeSize() const
		{
			return sizeof(FElementType);
		}

		/**
		 * Helper function to return the amount of memory allocated by this container.
		 * Only returns the size of allocations made directly by the container, not the elements themselves.
		 *
		 * @returns Number of bytes allocated by this container.
		 */
		FORCEINLINE PlatformSizeType GetAllocatedSize(void) const
		{
			return AllocatorInstance.GetAllocatedSize(ArrayMax, sizeof(FElementType));
		}

		/**
		 * Returns the amount of slack in this array in elements.
		 */
		FORCEINLINE FSizeType GetSlack() const
		{
			return ArrayMax - ArrayNum;
		}

	public:

		// Memory functions.

		/**
		 * Shrinks the array's used memory to smallest possible to store elements currently in it.
		 *
		 * @see Slack
		 */
		FORCEINLINE void Shrink()
		{
			CheckInvariants();
			if (ArrayMax != ArrayNum)
			{
				ResizeTo(ArrayNum);
			}
		}

	private:

		// Memory functions.

		/**
		 *
		 */
		FORCENOINLINE void ResizeGrow(FSizeType OldNum)
		{
			ArrayMax = AllocatorInstance.CalculateSlackGrow(ArrayNum, ArrayMax, sizeof(FElementType));
			AllocatorInstance.ResizeAllocation(OldNum, ArrayMax, sizeof(FElementType));
		}

		/**
		 *
		 */
		FORCENOINLINE void ResizeForCopy(FSizeType NewMax, FSizeType PrevMax)
		{
			if (NewMax != PrevMax)
			{
				AllocatorInstance.ResizeAllocation(0, NewMax, sizeof(FElementType));
			}

			ArrayMax = NewMax;
		}

		/**
		 *
		 */
		FORCENOINLINE void ResizeShrink()
		{
			const FSizeType NewArrayMax = AllocatorInstance.CalculateSlackShrink(ArrayNum, ArrayMax, sizeof(FElementType));

			if (NewArrayMax != ArrayMax)
			{
				ArrayMax = NewArrayMax;
				Check(ArrayMax >= ArrayNum);

				AllocatorInstance.ResizeAllocation(ArrayNum, ArrayMax, sizeof(FElementType));
			}
		}

		/**
		 *
		 */
		FORCENOINLINE void ResizeTo(FSizeType NewMax)
		{
			if (NewMax != ArrayMax)
			{
				ArrayMax = NewMax;
				AllocatorInstance.ResizeAllocation(ArrayNum, ArrayMax, sizeof(FElementType));
			}
		}

		/**
		 * Adds a given number of uninitialized elements into the array.
		 *
		 * Caution, AddUninitialized() will create elements without calling
		 * the constructor and this is not appropriate for element types that
		 * require a constructor to function properly.
		 *
		 * @param Count Number of elements to add.
		 * @returns Number of elements in array before addition.
		 */
		FORCEINLINE FSizeType AddUninitialized(FSizeType Count = 1)
		{
			CheckInvariants();
			Check(Count >= 0);

			const FSizeType OldNum = ArrayNum;

			if ((ArrayNum += Count) > ArrayMax)
			{
				ResizeGrow(OldNum);
			}

			return OldNum;
		}

		/**
		 *
		 */
		template <typename FOtherSizeType>
		void InsertUninitialized(FSizeType Index, FOtherSizeType Count)
		{
			CheckInvariants();
			Check((Count >= 0) & (Index >= 0) & (Index <= ArrayNum));

			FSizeType NewNum = Count;
			Check(static_cast<FOtherSizeType>(NewNum) == Count);


			const FSizeType OldNum = ArrayNum;

			if ((ArrayNum += Count) > ArrayMax)
			{
				ResizeGrow(OldNum);
			}

			FElementType* Data = GetData() + Index;
			RelocateConstructItems<FElementType>(Data + Count, Data, OldNum - Index);
		}

		/**
		 * Constructs a range of items into memory from a set of arguments.  The arguments come from an another array.
		 *
		 * @param	Dest		The memory location to start copying into.
		 * @param	Source		A pointer to the first argument to pass to the constructor.
		 * @param	Count		The number of elements to copy.
		 */
		template <typename FDestinationElementType, typename FSourceElementType>
		FORCEINLINE void ConstructItems(void* Dest, const FSourceElementType* Source, FSizeType Count)
		{
			if constexpr (TIsBitwiseConstructible<FDestinationElementType, FSourceElementType>::Value)
			{
				FMemory::Memcpy(Dest, Source, sizeof(FSourceElementType) * Count);
			}
			else
			{
				while (Count)
				{
					new (Dest) FDestinationElementType(*Source);
					++(FDestinationElementType*&)Dest;

					++Source;
					--Count;
				}
			}
		}

		/**
		 * Destructs a range of items in memory.
		 *
		 * @param	Elements	A pointer to the first item to destruct.
		 * @param	Count		The number of elements to destruct.
		 *
		 * @note: This function is optimized for values of T, and so will not dynamically dispatch destructor calls if T's destructor is virtual.
		 */
		FORCEINLINE void DestructItems(FElementType* Element, FSizeType Count)
		{
			if constexpr (!TIsTriviallyDestructible<FElementType>::Value)
			{
				while (Count)
				{
					using FDestructItemsElementType = FElementType;
					Element->FDestructItemsElementType::~FDestructItemsElementType();

					++Element;
					--Count;
				}
			}
		}

		/**
		 * Relocates a range of items to a new memory location as a new type. This is a so-called 'destructive move' for which
		 * there is no single operation in C++ but which can be implemented very efficiently in general.
		 *
		 * @param	Dest		The memory location to relocate to.
		 * @param	Source		A pointer to the first item to relocate.
		 * @param	Count		The number of elements to relocate.
		 */
		template <typename FDestinationElementType, typename FSourceElementType>
		FORCEINLINE void RelocateConstructItems(void* Dest, const FSourceElementType* Source, FSizeType Count)
		{
			if constexpr (TIsBitwiseRelocatable<FDestinationElementType, FSourceElementType>::Value)
			{
				FMemory::Memmove(Dest, Source, sizeof(FSourceElementType) * Count);
			}
			else
			{
				while (Count)
				{
					using FRelocateConstructItemsElementType = FSourceElementType;

					new (Dest) FDestinationElementType(*Source);
					++(FDestinationElementType*&)Dest;

					(Source++)->FRelocateConstructItemsElementType::~FRelocateConstructItemsElementType();
					--Count;
				}
			}
		}

		/**
		 *
		 */
		FORCEINLINE bool CompareItems(const FElementType* A, const FElementType* B, FSizeType Count)
		{
			if constexpr (TTypeTraits<FElementType>::IsBytewiseComparable)
			{
				return !FMemory::Memcmp(A, B, sizeof(FElementType) * Count);
			}
			else
			{
				while (Count)
				{
					if (!(*A == *B))
					{
						return false;
					}

					++A;
					++B;
					--Count;
				}

				return true;
			}
		}

		/**
		 * Copies data from one array into this array. Uses the fast path if the
		 * data in question does not need a constructor.
		 *
		 * @param Source The source array to copy
		 * @param PrevMax The previous allocated size
		 * @param ExtraSlack Additional amount of memory to allocate at
		 *                   the end of the buffer. Counted in elements. Zero by
		 *                   default.
		 */
		template <typename FOtherElementType, typename FOtherSizeType>
		void CopyToEmpty(const FOtherElementType* OtherData, FOtherSizeType OtherNum, FSizeType PrevMax, FSizeType ExtraSlack)
		{
			FSizeType NewNum = OtherNum;

			Check(static_cast<FOtherSizeType>(NewNum) == OtherNum);
			Check(ExtraSlack >= 0);

			ArrayNum = NewNum;

			if (OtherNum || ExtraSlack || PrevMax)
			{
				ResizeForCopy(NewNum + ExtraSlack, PrevMax);
				ConstructItems<FElementType, FOtherElementType>(GetData(), OtherData, OtherNum);
			}
			else
			{
				ArrayMax = 0;
			}
		}

		/**
		 *
		 */
		void RemoveAtImpl(FSizeType Index, FSizeType Count, bool bAllowShrinking)
		{
			if (Count)
			{
				CheckInvariants();
				Check((Count >= 0) & (Index >= 0) & (Index + Count <= ArrayNum));

				DestructItems(GetData() + Index, Count);
				FSizeType NumToMove = ArrayNum - Index - Count;

				if (NumToMove)
				{
					FMemory::Memmove
					(
						static_cast<uint8*>(AllocatorInstance.GetAllocation()) + (Index) * sizeof(FElementType),
						static_cast<uint8*>(AllocatorInstance.GetAllocation()) + static_cast<PlatformSizeType>(Index + Count) * sizeof(FElementType),
						NumToMove * sizeof(FElementType)
					);
				}

				ArrayNum -= Count;
				if (bAllowShrinking)
				{
					ResizeShrink();
				}
			}
		}

	private:

		// Move functions.

		/**
		 * Moves or copies array. Depends on the array type traits.
		 *
		 * This override moves.
		 *
		 * @param ToArray Array to move into.
		 * @param FromArray Array to move from.
		 */
		template <typename FFromArrayType, typename FToArrayType>
		static FORCEINLINE typename TEnableIf<Private::TCanMoveTArrayPointersBetweenArrayTypes<FFromArrayType, FToArrayType>::Value>::Type MoveOrCopy(FToArrayType& ToArray, FFromArrayType& FromArray, FSizeType PrevMax)
		{
			ToArray.AllocatorInstance.MoveToEmpty(FromArray.AllocatorInstance);

			ToArray.ArrayNum = FromArray.ArrayNum;
			ToArray.ArrayMax = FromArray.ArrayMax;
			FromArray.ArrayNum = 0;
			FromArray.ArrayMax = 0;
		}

		/**
		 * Moves or copies array. Depends on the array type traits.
		 *
		 * This override copies.
		 *
		 * @param ToArray Array to move into.
		 * @param FromArray Array to move from.
		 * @param ExtraSlack Tells how much extra memory should be preallocated
		 *                   at the end of the array in the number of elements.
		 */
		template <typename FFromArrayType, typename FToArrayType>
		static FORCEINLINE typename TEnableIf<!Private::TCanMoveTArrayPointersBetweenArrayTypes<FFromArrayType, FToArrayType>::Value>::Type MoveOrCopy(FToArrayType& ToArray, FFromArrayType& FromArray, FSizeType PrevMax)
		{
			ToArray.CopyToEmpty(FromArray.GetData(), FromArray.Num(), PrevMax, 0);
		}

	private:

		// Helper functions.

		/**
		 * Checks array invariants: if array size is greater than zero and less
		 * than maximum.
		 */
		FORCEINLINE void CheckInvariants() const
		{
			Check((ArrayNum >= 0) & (ArrayMax >= ArrayNum));
		}

		/**
		 * Checks if index is in array range.
		 *
		 * @param Index Index to check.
		 */
		FORCEINLINE void RangeCheck(FSizeType Index) const
		{
			CheckInvariants();

			if constexpr (FAllocator::RequireRangeCheck)
			{
				Check((Index >= 0) & (Index < ArrayNum));
			}
		}

		/**
		 * Checks that the specified address is not part of an element within the
		 * container. Used for implementations to check that reference arguments
		 * aren't going to be invalidated by possible reallocation.
		 *
		 * @param Addr The address to check.
		 * @see Add, Remove
		 */
		FORCEINLINE void CheckAddress(const FElementType* Address) const
		{
			Check(Address < GetData() || Address >= (GetData() + ArrayMax));
		}


	private:

		FAllocator AllocatorInstance;

		FSizeType ArrayNum;
		FSizeType ArrayMax;

	};

}