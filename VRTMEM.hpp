#ifndef _VRTMEM_
   #define _VRTMEM_

////////////////////////////////////////////////////////////////////////////
//
// Definition module: VMC  VRTMEM Virtual memory control
//
// Generated file:    VRTMEM.HPP
//
// Module identification letters: VMC
// Module identification number:  455
//
// Repository name:      Virtual memory
// Repository file name: Z:\TALISMAN\REPOSIT\BSW\VRTMEM.BSW
//
// Owning organization:    LES/DI/PUC-Rio
// Project:                Talisman
// List of authors
//    Id      Name
//    avs  - Arndt von Staa
//
// Software base change control
//       Version  Date         Authors      Description 
//       4     16/ago/2014  avs          Strenghtening verification and in memory access
//       3     23/jul/2012  avs          Adding open page control
//       2     07/set/2001  avs          Refactoring and adapting to new "virtual machine"
//       1     19/dez/2000  avs          Development begun / reusing Talisman 4.3
// -------------------------------------------------------------------------
// Specification
//    This module implements virtual page control classes.
//    
//    Virtual addresses are given by: < idSegment , idPage , offset >
//       idSegment - identifies the segment containing the page
//                   see module Segment for details.
//       idPage    - identifies the page within the segment
//                    all pages are of the same size: TAL_PageSize
//       offset    - is the first byte (char) of the data within the page.
//    
//    The virtual memory control provides real memory access to virtual pages,
//    where these pages are identified by <idSegment , idPage >
//    
//    Every virtual page belongs to a given segment.
//    An unbounded number of segments may be used.
//    
//    A segment contains 0 or more virtual pages.
//    
//    The segment identifier is defined when the segment is opened.
//    The value of a segment identifier is ephemeral.
//    It persists only while the corresponding segment object is available.
//    Once the segment object is deleted the idSegment ceases to be valid.
//    If a new segment object is created, available segment ids will be reused.
//    Hence recreating a segment object refering to the same segment file
//    does not assure that the idSsegment will be the same as the one
//    used in a previous usage instance of this file.
//    
//    See module Segment for more details.
//    
//    To access the contents of a virtual page it must first be paged in
//    into a page frame.
//    Page frames reside in real memory and contain a copy of the virtual
//    page value that is currently bound to it.
//    Page frames identify the segment and page of the page value, as well as
//    whether the page value has been changed and not yet written (dirty),
//    and whether it is pinned, i.e. may not be used when replacing pages.
//    
//    All page frames are members of a LRU (least recently used) list.
//    The head of this list is kept in the virtual memory root.
//    The list is ordered in order of recently used.
//    Most recently accessed page frames are at the beginning of the list,
//    empty frames and old page frames are at the tail of the list.
//    Page frames are moved within the list when they are accessed
//    by the VMC_GetPageFrame( idSeg, idPag ) function.
//    
//    When accessing the content of page values bound to some page frame
//    directly by a pointer they are not moved within the LRU list.
//    This may lead frames bound to pointed to pages being slowly moved
//    towards the end of the LRU list.
//    To prevent such pages to be removed, they should be pinned, i.e.
//    marked as non moveable.
//    However, pages should be pinned for short periods of time, and only
//    few pages should be pinned at each given time.
//    Since the number of memory resident frames is relatively small,
//    not following these rules may lead to "out of frame exceptions",
//    which are not recoverable.
//    
//    When a page is required first empty frames will be selected.
//    If none exists, the oldest non pinned page will be replaced by the
//    requested one.
//    
//    Page frames in use, i.e. those that contain a page, are kept in a
//    colision lists.
//    These lists are used by a search table to find the page frame
//    containing the page corresponding to a given virtual page address.
//    The heads of the colision lists are kept in a hash table.
//    
//    Whenever the contents of a page value are changed, the page frame
//    must be marked dirty.
//    Doing so will assure that the page value is written out to the
//    corresponding segment, either when removing the page from the frame,
//    or when the WriteAllFrames( ) method is called.
//    If changed pages are not set dirty, the contents of the segment
//    may become inconsistent.
//    There are several levels of dirty pages, see the
//    TAL_tpChangeLevel enumeration for more details.
//
////////////////////////////////////////////////////////////////////////////
// 
// Public methods of class VMC_PageFrame
// 
//    VMC_PageFrame( int inxPageFrameElemParm ,
//                   VMC_PageFrameElement * pFrameElementParm )
// 
//    ~VMC_PageFrame( )
// 
//    int VerifyPageFrame( TAL_tpVerifyMode  verifyMode )
// 
//    void DisplayPageFrame( int BytesPerLine = 10 )
// 
//    void DisplayPageFrame( int inxByteFirst ,
//                           int inxByteLast  ,
//                           int BytesPerLine = 10 )
// 
//    void DisplayPageFrameHeader( LOG_Logger * pLogger )
// 
//    void ReadPageFrame( int idSeg ,
//                        int idPag  )
// 
//    void WritePageFrame( )
// 
//    void PinFrame( )
// 
//    void UnpinFrame( )
// 
//    void RemoveAllPins( )
// 
//    void SetFrameEmpty( )
// 
//    void SetPageValueUndefined( )
// 
//    void SetIdSeg( int idSeg )
// 
//    void SetIdPag( int idPagParm )
// 
//    void SetFrameDirty( TAL_tpChangeLevel level = TAL_CHANGED )
// 
//    void SetPageData( const int offset ,
//                      const int length ,
//                      void  * pData    ,
//                      TAL_tpChangeLevel level = TAL_CHANGED )
// 
//    void GetPageData( const int offset ,
//                      const int length ,
//                      void  * pData    )
// 
//    char * GetPageValue( )
// 
//    int GetIdSeg( )
// 
//    int GetIdPag( )
// 
//    int GetInxPageFrameElem( )
// 
//    int GetNumPins( )
// 
//    TAL_tpChangeLevel GetDirtyFlag( )
// 
//    STR_String * GetSegmentFileName( )
// 
//    STR_String * GetSegmentFullName( )
// 
// Public methods of class VMC_VirtualMemoryRoot
// 
//    void CreateRoot( int minFrames ,
//                     int maxFrames  )
// 
//    void DestroyRoot( )
// 
//    VMC_VirtualMemoryRoot * GetRoot( )
// 
//    bool IsPageInMemory( int idSeg ,
//                         int idPag  )
// 
//    int VerifyVirtualMemory( TAL_tpVerifyMode verifyMode )
// 
//    int CountOpenPages( int idSeg )
// 
//    int VerifyOpenPages( TAL_tpVerifyMode verifyMode )
// 
//    void DisplayStatistics( )
// 
//    void DisplayPinnedFrameList( )
// 
//    void WriteAllPageFrames( )
// 
//    void RemoveSegment( int idSeg )
// 
//    VMC_PageFrame * AddNewPage( int idSeg )
// 
//    VMC_PageFrameElement * GetLruListHead( )
// 
//    VMC_PageFrameElement * GetNextLruListElement( VMC_PageFrameElement * currentElem )
// 
//    VMC_PageFrame * GetPageFrame( int  idSeg ,
//                                  int  idPag ,
//                                  bool inMemory = false )
// 
//    int GetNumPageFrames( )
// 
//    int GetTotalAccesses( )
// 
//    int GetTotalReplaces( )
// 
//    int GetTotalHits( )
// 
//    int GetNumPinnedFrames( )
// 
//    VMC_PageFrame * GetPageFrame( VMC_PageFrameElement * currentElem )
// 
// 
// -------------------------------------------------------------------------
// Protected methods of class VMC_PageFrame
// 
// -------------------------------------------------------------------------
// Protected methods of class VMC_VirtualMemoryRoot
// 
//    VMC_VirtualMemoryRoot( int minFramesParm ,
//                           int maxFramesParm  )
// 
//    ~VMC_VirtualMemoryRoot( )
// 
// 
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// Error log message codes of the module
//
// Method VMF !Verify page frame object
// 
// Error log codes
//    50 - wrong page value underflow control
//    51 - wrong page value overflow control
//    52 - segment id is not in use
//    53 - frame in use contains negative page id
//    54 - page id is larger than the size of the segment
//    55 - negative number of pins
//    56 - number of pins larger than maximum allowed
//    57 - wrong change level
//    58 - incorrect empty frame
//    59 - if a frame is pinned it must contain a page
//
// Method VMR !Verify virtual memory root object
// 
// Error log codes
//     1 - null root object pointer
//     2 - incorrect root object pointer
//     3 - segment control is not open
//     4 - colision list refers to frame element that is not in use
//     5 - colision list element contains incorrect hash index
//     6 - colision list page frame contains negative segment id
//     7 - colision list page frame contains negative page id
//     8 - colision list element contains incorrect virtual address
//     9 - colision list element contains incorrect forward pointer
//    10 - colision list element contains incorrect backward pointer
//    11 - number of page frames less than minimum required
//    12 - first LRU list element contains backward refernce
//    13 - incorrect LRU list head
//    14 - last LRU list element contains forward refernce
//    15 - incorrect LRU list tail
//    16 - incorrect next LRU list pointer
//    17 - incorrect sucessor frame index
//    18 - last LRU element is not LRU list tail
//    19 - last LRU element index is not 0
//    20 - incorrect backward LRU list pointer
//    21 - incorrect predecessor frame index
//    22 - first LRU element is not LRU list head
//    23 - first LRU element index is not last frame element index
//    24 - page frame element in use contains negative hash index
//    25 - page frame element in use contains too large hash index
//    26 - page frame element in use contains incorrect hash index
//    27 - page frame element type is incorrect
//    28 - free page frame element contains positive hash index
//    29 - free page frame element contains positive segment id
//    30 - free page frame element contains positive page id
//    31 - page frame index is different from page frame element index
//    32 - page frame object is incorrect
//    33 - incorrect number of open pages upon entry
//    34 - incorrect number of open pages upon exit
//
////////////////////////////////////////////////////////////////////////////

//==========================================================================
//----- Required includes -----
//==========================================================================

   #include <stdio.h>
   #include "talisman_constants.inc"
   #include "segment.hpp"
   #include "logger.hpp"

//==========================================================================
//----- Exported declarations -----
//==========================================================================

   enum   VMC_tpFrameType  ;
   struct VMC_PageFrameElement ;


//==========================================================================
//----- Class declaration -----
//==========================================================================


////////////////////////////////////////////////////////////////////////////
// 
//  Class: VMF  Page frame
// 
// Description
//    Page frames reside in real memory.
//    They may contain a copy of the a value contained in a segment.
//    When a page is modified its corresponding page frame should be marked
//    dirty, otherwise the changes might be lost.
//    While it is dirty the page value in the frame is different from the
//    value of the virtual page contained in the segment.
//    Writing dirty pages is not synchronous with the changes being made.
//    Usually writting occurs after the end of a transaction, or when
//    the contents of the page frame is replaced.
//    
//    Objects that manipulate virtual pages may keep a pointer to the
//    corresponding page frame.
//    Hence, it may happen that such an object refers to a frame that will
//    be used when binding the pagae frame to another virtual page, leading
//    to serious damages of the segment content.
//    To prevent this, page frames must pe pinned whenever such a pointer
//    is defined.
//    The frame pin counter should always be equal to the number of
//    active pointers to a frame.
// 
////////////////////////////////////////////////////////////////////////////

class VMC_PageFrame
{

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Page frame empty constructor
// 
// Description
//    Should only be used by the virtual memory components.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      VMC_PageFrame( int inxPageFrameElemParm ,
                     VMC_PageFrameElement * pFrameElementParm )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Virtual Method: VMF !Page frame destructor
// 
// Description
//    Should only be used by the virtual memory components.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      virtual ~VMC_PageFrame( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Verify page frame object
// 
// Description
//    Verifies the integrity of a given frame.
// 
// Parameters
//    $P ModeParm - TAL_VerifyLog generates a log of all problems
//                                found
//                  TAL_VerifyNoLog returns when encountering the
//                                  first problem
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int VerifyPageFrame( TAL_tpVerifyMode  verifyMode )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Display frame
// 
// Description
//    Displays the page frame contents
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void DisplayPageFrame( int BytesPerLine = 10 )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Display part of page frame
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void DisplayPageFrame( int inxByteFirst ,
                             int inxByteLast  ,
                             int BytesPerLine = 10 )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Display page frame header
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void DisplayPageFrameHeader( LOG_Logger * pLogger )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Read page value into frame
// 
// Description
//    Should only be used by the virtual memory components.
// 
// Returned exceptions
//    May pass on exceptions thrown while reading
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void ReadPageFrame( int idSeg ,
                          int idPag  )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Write page value contained in frame
// 
// Description
//    Writes the page contained in the page frame.
//    The page will be written only if it is dirty.
//    The page to be written must be one already existing in the segment.
// 
// Returned exceptions
//    EXC_Error - may occur when attempting to write on a read only file,
//                     These exceptions are ignored if the recorded change
//                     is "ignorable".
//               - may also occur due to I/O errors
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void WritePageFrame( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Add a pin to a frame
// 
// Description
//    PinFrame increses the number of pins.
//    UnPinFrame reduces the number of pins.
//    
//    Page values in a pinned page frame cannot be removed when searching
//    for page frame to recieve a replacement page.
//    
//    Virtual memory does not assure that pages remain at the same
//    physical memory location over some period of time.
//    Thus if active pointers to within a page value are used for some time,
//    the corresponding page frame must be pinned.
//    Whenever the pointer ceases to be active, the frame should be unpinned.
//    
//    Physical memory addresses may be required to increase performance,
//    since this permits access to page contents without having to
//    translate virtual addresses to physical real addresses.
//    However, they should only be used by short transactions that make
//    heavy use of access to the page value contained in the frame.
//    
//    If pins are used in int transactions, crashes may occur due to
//    unavailable page frames for page replacements.
//    
//    numPinnedFrames keeps a statistic of the maximum number of
//    simultaneously pinned frames during a virtual memory usage session.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void PinFrame( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Remove a pin from the frame
// 
// Description
//    Removes a pin.
//    See PinFrame for details.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void UnpinFrame( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Remove all pins from the frame
// 
// Description
//    Removes all frame pins.
//    See PinFrame for details.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void RemoveAllPins( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Set frame empty
// 
// Description
//    Should only be used by the virtual memory components.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void SetFrameEmpty( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Set page value to undefined chars
// 
// Description
//    Should only be used by the virtual memory components.
//    Sets the change level to TAL_CHANGED
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void SetPageValueUndefined( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Set segment identification
// 
// Description
//    Should only be used by the virtual memory components.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void SetIdSeg( int idSeg )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Set page identification
// 
// Description
//    Should only be used by the virtual memory components.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void SetIdPag( int idPagParm )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Set frame dirty
// 
// Description
//    Inserts the frame in the list of pages to be written.
//    Only pages belonging to dirty frames list are written to a file.
//    Workspaces are not set to dirty.
// 
// Parameters
//    $P Level - level of the change, see the TAL_tpChangeLevel
//               enumeration for details.
// 
// Returned exceptions
//    ERROR VMC_ErrorReadOnly  if the segment is read only and the
//    level parameter is not VMC_IgnorableChange
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void SetFrameDirty( TAL_tpChangeLevel level = TAL_CHANGED )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Set page data
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void SetPageData( const int offset ,
                        const int length ,
                        void  * pData    ,
                        TAL_tpChangeLevel level = TAL_CHANGED )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: WMF !Get page data
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void GetPageData( const int offset ,
                        const int length ,
                        void  * pData    )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Get page value pointer
// 
// Return value
//    Returns the pointer to the page value of the frame
//            NULL if no page value is bound to the frame
// 
////////////////////////////////////////////////////////////////////////////

   public:
      char * GetPageValue( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Get segment id
// 
// Description
//    Returns the segment identification of the page contained in the frame
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int GetIdSeg( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Get page id
// 
// Description
//    Returns the identification of the page contained in the frame.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int GetIdPag( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Get page frame element index
// 
// Description
//    Should only be used by the virtual memory components.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int GetInxPageFrameElem( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Get number of pins of the frame
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int GetNumPins( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Get dirty flag
// 
////////////////////////////////////////////////////////////////////////////

   public:
      TAL_tpChangeLevel GetDirtyFlag( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Get segment file name
// 
////////////////////////////////////////////////////////////////////////////

   public:
      STR_String * GetSegmentFileName( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMF !Get segment full name
// 
////////////////////////////////////////////////////////////////////////////

   public:
      STR_String * GetSegmentFullName( )  ;

////////////////////////////////////////////////////////////////////////////

// VMF index of the page frame element
//    This index is redundant, it is used by structural verifiers.

   private: 
      int inxPageFrameElem ;

// VMF corresponding frame element

   private: 
      VMC_PageFrameElement * pFrameElement ;

// VMF Page value underflow control
//    protctionBefore and protectionAfter control possible buffer overrun
//    that might occur when transferring data to a page.
//    The control has a very slight chance of not working, depending
//    on the data that is transferred into the page.

   private: 
      static const int PROTECTION_SIZE = 4 ;
      char protectionBefore[ PROTECTION_SIZE ] ;

// VMF Page value copy of the virtual page
//    This space contains the value of the virtual page.
//    Its value might different from the one in the corresponding segment,
//    up to the moment the page is written out to the segment.
//    To assure that it will be written, page frames must be marked dirty
//    whenever the page contents is changed.

   private: 
      char pageValue[ TAL_PageSize ] ;

// VMF Page value overflow control

   private: 
      char protectionAfter[ PROTECTION_SIZE ] ;

// VMF Segment identifier
//    This identifier is generated by the segment module.
//    Its value is ephemeral in the sense that different usage sessions
//    may yield different identifiers for a same segment file.
//    
//    If the page frame is empty the identifier should be NULL_SEGMENT

   private: 
      int idSegment ;

// VMF Page identifier
//    This identifier is the index of the page within the segment file.
//    
//    If the page frame is empty the identifier should be NULL_PAGE

   private: 
      int idPage ;

// VMF Change level

   private: 
      TAL_tpChangeLevel changeLevel ;

// VMF Number of pins
//    Whenever a page frame is pinned, this counter is increased.
//    When the page frame is unpinned the counter is decreased.
//    When the number is non zero, the page frame must not be chosen
//    for replacement of its page value.
//    Frames should be pinned whenever an active page value pointer is
//    defined, and should be unpinned whenever the pointer ceases to be
//    active.

   private: 
      int numPins ;

} ; // End of class declaration: VMF  Page frame


//==========================================================================
//----- Class declaration -----
//==========================================================================


////////////////////////////////////////////////////////////////////////////
// 
//  Class: VMR  Virtual memory root singleton
// 
// Description
//    The virtual memory root singleton is responsible for all
//    page frame access operations.
//    
//    The root class contains the roots to the lists of page frames.
// 
////////////////////////////////////////////////////////////////////////////

class VMC_VirtualMemoryRoot
{

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !:Virtual memory root create
// 
// Description
//    This function creates the singleton.
// 
// Parameters
//    $P minFrames - is the minimum number of frames that must be allocated.
//    $P maxFrames - is the maximum number of frames that may  be allocated.
// 
// Returned exceptions
//    Assertion - if the singleton exists
// 
////////////////////////////////////////////////////////////////////////////

   public:
      static void CreateRoot( int minFrames ,
                              int maxFrames  )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !:Virtual memory root delete
// 
////////////////////////////////////////////////////////////////////////////

   public:
      static void DestroyRoot( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Inline Method: VMR !:Get virtual memory root
// 
// Returned exceptions
//    Assert - if the singleton has not been created
// 
////////////////////////////////////////////////////////////////////////////

   public:
      static inline VMC_VirtualMemoryRoot * VMC_VirtualMemoryRoot :: GetRoot( )
      {

         return pVirtualMemoryRoot ;

      } // End : Root of VMR !:Get virtual memory root

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Is page in memory
// 
// Description
//    Returns true if the page is already in memory, or if there is
//    an empty frame that could receive the virtual memory
// 
////////////////////////////////////////////////////////////////////////////

   public:
      bool IsPageInMemory( int idSeg ,
                           int idPag  )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Verify virtual memory root object
// 
// Description
//    Verifies all frames that are registered in the LRU list.
// 
// Parameters
//    $P ModeParm - TAL_VerifyLog   - generates a log of all errors found
//                  TAL_VerifyNoLog - returns upon finding the first error
// 
// Return value
//    true  - if the virtual memory structure is correct
//    false - if it is corrupted
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int VerifyVirtualMemory( TAL_tpVerifyMode verifyMode )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Verify number of open pages
// 
// Description
//    Counts open pages of a given segment paged in to the LRU list
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int CountOpenPages( int idSeg )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Verify all open pages
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int VerifyOpenPages( TAL_tpVerifyMode verifyMode )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Display virtual memory usage statistics
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void DisplayStatistics( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Display list of pinned frames
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void DisplayPinnedFrameList( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Write all dirty frames
// 
// Description
//    Writes all dirty pages to their corresponding virtual page.
//    After writing all pages are not dirty.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void WriteAllPageFrames( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Remove all pages of a given segment
// 
// Description
//    Removes all pages that belong to a given segment.
//    This method should be used before deleting a segment.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      void RemoveSegment( int idSeg )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Add new page value to end of segment file
// 
// Description
//    This function gets a frame containing a new empty page added to the end
//    of the segment file.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      VMC_PageFrame * AddNewPage( int idSeg )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Get LRU list head
// 
// Description
//    Should only be used by the virtual memory components.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      VMC_PageFrameElement * GetLruListHead( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Get next LRU list element
// 
// Description
//    Should only be used by the virtual memory components.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      VMC_PageFrameElement * GetNextLruListElement( VMC_PageFrameElement * currentElem )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Get page frame
// 
// Description
//    This method returns the page frame where a copy of the virtual pages
//    can be found.
//    Only one copy of the virtual page may existe in real memory.
//    Hence, all simmultaneously existing references to a same
//    virtual address will map onto the same page frame.
//    The location of virtual pages is ephemeral, if a page frame is not
//    pinned it might be selected to be paged out. Hence, on the next
//    access to the virtual page probably the page frame holding the
//    virtual page will be a different one.
//    An exception is generated if no repleceable frame can be found.
//    This occurs if all page frames are pinned.
//    
//    Certain methods restrict access to pages that are already in memory.
//    To satisfy this restriction use  inMemory == true.
//    When accessing virtual pages that must be in memory, following
//    rules hold:
//    - if the virtual page is already in memory, its page frame
//      is returned, but the page is not moved to the head of the
//      LRU list.
//    - if the virtual page is not in memory, but the last element
//      of the LRU list is a free page, the virtual page is read into
//      the corresponding and the page frame is moved to the head of
//      the LRU list.
//    - if none of these conditions hold, NULL is returned
// 
// Parameters
//    $P idSeg
//    $P idPag - virtual address of the virtual page to be placed
//               in real memory
//    $P inMemory - if true only virtual pages already in memory
//                  or copied into a free frame are accessed
//                - if false replaces least recently used pages whenever
//                  necessary.
// 
// Return value
//    Pointer to the page frame containing a copy of the virtual page.
// 
// Returned exceptions
//    EXC_Error if page does not exist.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      VMC_PageFrame * GetPageFrame( int  idSeg ,
                                    int  idPag ,
                                    bool inMemory = false )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Get number page frames
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int GetNumPageFrames( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Get total frame accesses
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int GetTotalAccesses( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Get total frame replaces
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int GetTotalReplaces( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Get total hit count
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int GetTotalHits( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Get number of pinned frames
// 
////////////////////////////////////////////////////////////////////////////

   public:
      int GetNumPinnedFrames( )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR !Get page frame of current element
// 
// Description
//    Should only be used by the virtual memory components.
// 
////////////////////////////////////////////////////////////////////////////

   public:
      VMC_PageFrame * GetPageFrame( VMC_PageFrameElement * currentElem )  ;

////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR #Virtual memory root constructor
// 
////////////////////////////////////////////////////////////////////////////

   protected:
      VMC_VirtualMemoryRoot( int minFramesParm ,
                             int maxFramesParm  )  ;

////////////////////////////////////////////////////////////////////////////
// 
//  Virtual Method: VMR #Virtual memory root destructor
// 
////////////////////////////////////////////////////////////////////////////

   protected:
      virtual ~VMC_VirtualMemoryRoot( )  ;

////////////////////////////////////////////////////////////////////////////

//  Method: VMR $Search page frame element of virtual page already in real memory

   private:
      VMC_PageFrameElement * SearchRealPage( int idSeg ,
                                         int idPag  )  ;

//  Method: VMR $Start up virtual memory

   private:
      void StartUpVirtualMemory( int minFramesParm ,
                      int maxFramesParm  )  ;

//  Method: VMR $Find a replaceable page frame element

   private:
      VMC_PageFrameElement * FindReplaceableFrame( )  ;

//  Method: VMR $Move to LRU head the page frame element

   private:
      void MoveElemLruHead( VMC_PageFrameElement * pPageFrameElem )  ;

//  Method: VMR $Move to LRU tail the page frame element

   private:
      void MoveElemLruTail( VMC_PageFrameElement * pPageFrameElem )  ;

//  Method: VMR $Replace page in frame

   private:
      void ReplacePage( VMC_PageFrameElement * pPageFrameElem ,
                        int  idSeg    ,
                        int  idPag    ,
                        bool isNewPage )  ;

//  Method: VMR $Get empty page frame

   private:
      VMC_PageFrameElement * GetEmptyFrame( int idSeg ,
                                        int idPag  )  ;

//  Method: VMR $Remove page from frame

   private:
      void RemovePageValue( VMC_PageFrameElement * pPageFrameElem )  ;

//  Method: VMR $Compute hash index

   private:
      int ComputeInxHash( int idSeg , int idPag )  ;

//  Method: VMR $Link page frame into colision list

   private:
      void LinkColisionList( VMC_PageFrameElement * pPageFrameElem )  ;

//  Method: VMR $Unlink page frame from colision list

   private:
      void UnlinkColisionList( VMC_PageFrameElement * pPageFrameElem )  ;

//  Method: VMR $Verify and correct all page counters

   private:
      int VerifyCorrectOpenPageCount( )  ;

////////////////////////////////////////////////////////////////////////////

// VMR Hit counter

   private: 
      int totalHitCounter ;

// VMR Access counter

   private: 
      int totalAccessCounter ;

// VMR Replace counter

   private: 
      int totalReplaceCounter ;

// VMR LRU list anchor

   private: 
      VMC_PageFrameElement * lruListHead ;
      VMC_PageFrameElement * lruListTail ;

// VMR Hash table colision lists array

   private: 
      VMC_PageFrameElement * vtColision[ TAL_dimColision ] ;

// VMR Number of available frames

   private: 
      int numPageFrames ;

// VMR Virtual memory root pointer

   private: 
      static VMC_VirtualMemoryRoot * pVirtualMemoryRoot ;

} ; // End of class declaration: VMR  Virtual memory root singleton

#undef _VRTMEM_CLASS

#endif 

////// End of definition module: VMC  VRTMEM Virtual memory control ////

