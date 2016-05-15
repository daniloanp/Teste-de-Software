////////////////////////////////////////////////////////////////////////////
//
//Implementation module: VMC  VRTMEM Virtual memory control
//
//Generated file:        VRTMEM.CPP
//
//Module identification letters: VMC
//Module identification number:  455
//
//Repository name:      Virtual memory
//Repository file name: Z:\TALISMAN\REPOSIT\BSW\VRTMEM.BSW
//
//Owning organization:    LES/DI/PUC-Rio
//Project:                Talisman
//List of authors
//   Id      Name
//   avs  - Arndt von Staa
//
//Software base change control
//    Version  Date         Authors      Description 
//    4     16/ago/2014  avs          Strenghtening verification and in memory access
//    3     23/jul/2012  avs          Adding open page control
//    2     07/set/2001  avs          Refactoring and adapting to new "virtual machine"
//    1     19/dez/2000  avs          Development begun / reusing Talisman 4.3
//
////////////////////////////////////////////////////////////////////////////

   #include  <string.h>
   #include  <stdlib.h>

   #define  _VRTMEM_OWN
   #include "VRTMEM.hpp"
   #undef   _VRTMEM_OWN

  

   #include "exceptn.hpp"
   #include "message.hpp"
   #include "msgbin.hpp"
   #include "msgstr.hpp"
   #include "msgtime.hpp"
   #include "segmsg.hpp"
   #include "global.hpp"

   #include "str_vmc.inc"
   #include "str_global.inc"

//==========================================================================
//----- Encapsulated data types -----
//==========================================================================


////////////////////////////////////////////////////////////////////////////
// 
//  Data type: VMR Page frame element types
// 
////////////////////////////////////////////////////////////////////////////

   enum VMC_tpFrameType
   {

   // VMR Frame in use

      FRAME_TYPE_IN_USE ,

   // VMR Free frame

      FRAME_TYPE_FREE

   }  ;


////////////////////////////////////////////////////////////////////////////
// 
//  Data type: VMR Frame list element
//    Defines the frame list element.
//    All frames are part of the LRU and of colision lists.
// 
////////////////////////////////////////////////////////////////////////////

   struct VMC_PageFrameElement
   {

   // VMR Page frame identifier
//    Frames are stored in an array of frames contained in the virtual
//    memory root object.
//    This index corresponds to the index of the frame in this array.
//    The index is redundant and is used by structural verifiers.

      int inxFrameElement ;

   // VMR Hash index
//    Frames in use are part of some colision list.
//    This attribute tell in which colision list the page frame resides.

      int inxHash ;

   // VMR Previous colision list element
//    this pair of pointers establish the list of page frames containing
//    virtual addresses that are coherent with a given hash index.
//    The colision list is a non circular doubly linked list.
//    Colision lists are anchored in the hash table.

      VMC_PageFrameElement * prevColisionElem ;

   // VMR Colision list successor

      VMC_PageFrameElement * nextColisionElem ;

   // VMR LRU list predecessor
//    this pair of pointers links page frames in the LRU list.
//    The LRU list is a non circular doubly linked list.
//    The elements of this list are ordered by ascending age.
//    Age is set to 0 whenever a virtual page is accessed using the
//    GetPageFrame( idSegment, idPage) method.
//    The age is increased whenever an older frame is accessed by this
//    method.
//    nextLruElem follows the list in increasing age order,
//    prevLruElem does it in decreasing age order.
//    Empty frames are always placed at the end of the list.
//    The list is anchored in the virtual memory root.

      VMC_PageFrameElement * prevLruElem ;

   // VMR LRU list successor

      VMC_PageFrameElement * nextLruElem ;

   // VMR page frame contained in this element

      VMC_PageFrame * pPageFrame ;

   // VMR Frame type

      VMC_tpFrameType frameType ;

   // VMR Framelist element constructor

      VMC_PageFrameElement( int inxFrameElem )
      {
         inxFrameElement   = inxFrameElem ;
         inxHash           = -1 ;
         prevColisionElem = NULL ;
         nextColisionElem = NULL ;
         prevLruElem       = NULL ;
         nextLruElem       = NULL ;
         frameType         = FRAME_TYPE_FREE ;
         pPageFrame        = new VMC_PageFrame( inxFrameElem , this ) ;
      }

   // VMR Framelist element destructor

      ~VMC_PageFrameElement( )
      {
         delete pPageFrame ;

         inxFrameElement   = -1 ;
         inxHash           = -1 ;
         prevColisionElem = NULL ;
         nextColisionElem = NULL ;
         prevLruElem       = NULL ;
         nextLruElem       = NULL ;
         pPageFrame        = NULL ;
         frameType         = FRAME_TYPE_FREE ;
      }

   }  ;


//==========================================================================
//----- Encapsulated data items -----
//==========================================================================


// VMR Overflow control size

   static const int PROTECTION_SIZE = 4 ;

// VMR Page value buffer underflow protection constant

   static const char BEFORE[ ] = "\0\xF6\x3F\xF3" ;

// VMR Page value buffer overflow protection constant

   static const char AFTER[ ] = "\xF6\x3F\xF3\0" ;

// VMR Undefined page value character

   static const char VALUE_UNDEFINED = '+' ;  //  '\xFA' ;

// VMR Maximum number of pins of any frame

   static const int NUM_MAX_PINS = 100 ;

// VMR Minimum number of page frames

   static const int numMinFrames = 5 ;

// VMR Instantaneous number of simmultaneously pinned frames

   static int numPinnedFrames = 0 ;

// VMR Max number of simmultaneously pinned frames

   static int maxPinnedFrames = 0 ;

//==========================================================================
//----- Static member initializations -----
//==========================================================================

   // VMR Virtual memory root pointer

      VMC_VirtualMemoryRoot * VMC_VirtualMemoryRoot :: pVirtualMemoryRoot = NULL ;


//==========================================================================
//----- Class implementation -----
//==========================================================================

////////////////////////////////////////////////////////////////////////////
// 
// Implementation of class: VMF  Page frame
////////////////////////////////////////////////////////////////////////////

//==========================================================================
//----- Public method implementations -----
//==========================================================================

// Class: VMF  Page frame

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Page frame empty constructor

   VMC_PageFrame ::
             VMC_PageFrame( int inxPageFrameElemParm ,
                            VMC_PageFrameElement * pFrameElementParm )
   {

      memcpy( protectionBefore, BEFORE , PROTECTION_SIZE  ) ;
      memcpy( protectionAfter , AFTER  , PROTECTION_SIZE  ) ;
      inxPageFrameElem = inxPageFrameElemParm ;
      pFrameElement    = pFrameElementParm ;

      SetFrameEmpty( ) ;

   } // End of function: VMF !Page frame empty constructor

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Page frame destructor

   VMC_PageFrame ::
             ~VMC_PageFrame( )
   {

      SetFrameEmpty( ) ;

   } // End of function: VMF !Page frame destructor

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Verify page frame object

   #define   ASSERT_VER( Condition , idMsg )   \
      if ( !( Condition ) )                    \
      {                                        \
         if ( verifyMode == TAL_VerifyLog )    \
         {                                     \
            EXC_LOG( envelope.pMsg , idMsg ) ; \
            numErrors ++ ;                     \
         } else                                \
         {                                     \
            return 1 ;                         \
         } /* if */                            \
      }  /* if */

   int VMC_PageFrame ::
             VerifyPageFrame( TAL_tpVerifyMode  verifyMode )
   {

      int numErrors = 0 ;

      // Page frame verification setup

         struct PointerEnvelope
         {
            MSG_Message * pMsg ;

            PointerEnvelope( )
            {
               pMsg = NULL ;
            }

           ~PointerEnvelope( )
            {
               delete pMsg ;
            }
         } envelope ; /* struct */

         if ( verifyMode == TAL_VerifyLog )
         {
            envelope.pMsg = new MSG_Message( VMC_ErrorPageFrame ) ;
            envelope.pMsg->AddItem( 1 , new MSG_ItemInteger( inxPageFrameElem )) ;
            envelope.pMsg->AddItem( 2 , new MSG_ItemInteger( idPage    )) ;
            envelope.pMsg->AddItem( 3 , new SEG_ItemSegmentFullName( idSegment )) ;
         } /* if */

      // Verify frame page write overflow control

         ASSERT_VER( memcmp( protectionBefore , BEFORE , PROTECTION_SIZE ) == 0 , 50 ) ;
         ASSERT_VER( memcmp( protectionAfter  , AFTER  , PROTECTION_SIZE ) == 0 , 51 ) ;

      // Verify frame in use

         if ( idSegment >= 0 )
         {

            SEG_SegmentRoot * pRoot = SEG_SegmentRoot::GetRoot( ) ;

            // Verify virtual address of frame

               ASSERT_VER( pRoot->VerifyIdSeg( idSegment ) , 52 ) ;

         } // end selection: Verify frame in use

      // Verify empty frame

         else
         {

         } // end selection: Verify empty frame

      return numErrors ;

   } // End of function: VMF !Verify page frame object

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Display frame

   void VMC_PageFrame :: DisplayPageFrame( int bytesPerLine )
   {

      DisplayPageFrame( 0 , TAL_PageSize - 1 , bytesPerLine ) ;

   } // End of function: VMF !Display frame

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Display part of page frame

   void VMC_PageFrame ::
             DisplayPageFrame( int inxByteFirst ,
                               int inxByteLast  ,
                               int BytesPerLine )
   {

      if ( inxByteFirst < 0 )
      {
         inxByteFirst = 0 ;
      } /* if */

      if ( inxByteLast >= TAL_PageSize )
      {
         inxByteLast = TAL_PageSize - 1 ;
      } /* if */

      if ( inxByteFirst >= inxByteLast )
      {
         inxByteFirst = inxByteLast - 1;
      } /* if */

      LOG_Logger * pLogger = GLB_GetGlobal( )->GetEventLogger( ) ;
      pLogger->Log( "" ) ;

      DisplayPageFrameHeader( pLogger ) ;

      pLogger->LogDataSpace( inxByteFirst , inxByteLast + 1 ,
                             pageValue    , BytesPerLine ) ;
      pLogger->Log( "" ) ;

   } // End of function: VMF !Display part of page frame

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Display page frame header

   void VMC_PageFrame ::
             DisplayPageFrameHeader( LOG_Logger * pLogger )
   {

      char dirty[ 20 ] ;

      strcpy( dirty , STR_GetStringAddress( VMC_FormatNotDirty )) ;

      if ( changeLevel == TAL_CHANGED )
      {
         strcpy( dirty , STR_GetStringAddress( VMC_FormatIsDirty )) ;

      } else if ( changeLevel == TAL_IGNORABLE_CHANGE )
      {
         strcpy( dirty , STR_GetStringAddress( VMC_FormatIgnorable )) ;
      } /* if */

      char buffer[ 100 ] ;
      STR_String * pSegName = GetSegmentFullName( ) ;
      STR_ConvertToPrintable( pSegName->GetLength( ) , pSegName->GetString( ) ,
                30 , buffer , false ) ;
      delete pSegName ;

      int type = pFrameElement->frameType ;

      char msg[ 120 ] ;
      sprintf( msg , STR_GetStringAddress( VMC_FormatFrameHead ) ,
               inxPageFrameElem , buffer , idPage , numPins , dirty , type ) ;
      pLogger->Log( msg ) ;

   } // End of function: VMF !Display page frame header

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Read page value into frame

   void VMC_PageFrame ::
             ReadPageFrame( int idSeg ,
                            int idPag  )
   {


      SEG_SegmentRoot::GetRoot( )->ReadPage( idSeg , idPag , &pageValue ) ;

      idSegment   = idSeg ;
      idPage      = idPag ;

      changeLevel = TAL_NOT_CHANGED ;
      numPins     = 0 ;

   } // End of function: VMF !Read page value into frame

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Write page value contained in frame

   void VMC_PageFrame ::
             WritePageFrame( )
   {


      if ( changeLevel < TAL_NOT_CHANGED )
      {

         try
         {
            SEG_SegmentRoot::GetRoot( )->WritePage( idSegment , idPage , pageValue ) ;
            changeLevel = TAL_NOT_CHANGED ;
         }
         catch ( EXC_Exception * pExc )
         {
            if ( changeLevel == TAL_IGNORABLE_CHANGE )
            {
               changeLevel = TAL_NOT_CHANGED ;
               delete pExc ;
            } else
            {
               throw pExc ;
            } /* if */
         } /* end catch */

      } // end selection: Root of VMF !Write page value contained in frame

   } // End of function: VMF !Write page value contained in frame

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Add a pin to a frame

   void VMC_PageFrame :: PinFrame( )
   {


      if ( numPins >= NUM_MAX_PINS )
      {
         MSG_Message * pMsg = new MSG_Message( VMC_TooManyPins ) ;
         pMsg->AddItem( 0 , new MSG_ItemInteger( idPage )) ;
         pMsg->AddItem( 1 , new SEG_ItemSegmentFullName( idSegment )) ;
         EXC_PROGRAM( pMsg , -1 , TAL_NullIdHelp ) ;
      } /* if */

      if ( numPins == 0 )
      {
         numPinnedFrames ++ ;
         if ( numPinnedFrames > maxPinnedFrames )
         {
            maxPinnedFrames = numPinnedFrames ;
         } /* if */
      } /* if */

      numPins ++ ;

   } // End of function: VMF !Add a pin to a frame

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Remove a pin from the frame

   void VMC_PageFrame :: UnpinFrame( )
   {

      if ( numPins > 0 )
      {
         numPins -- ;
         if ( ( numPins   <= 0 )
           || ( idSegment <  0 ))
         {
            numPins = 0 ;
            numPinnedFrames -- ;
         } /* if */
      } /* if */

   } // End of function: VMF !Remove a pin from the frame

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Remove all pins from the frame

   void VMC_PageFrame :: RemoveAllPins( void )
   {


      if ( numPins > 0 )
      {
         numPinnedFrames -- ;
      } /* if */

      numPins = 0 ;

   } // End of function: VMF !Remove all pins from the frame

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Set frame empty

   void VMC_PageFrame ::
             SetFrameEmpty( )
   {


      SetPageValueUndefined( ) ;

      idSegment      = TAL_NullIdSeg ;
      idPage         = TAL_NullIdPag ;

      changeLevel    = TAL_NOT_CHANGED ;
      numPins        = 0 ;

   } // End of function: VMF !Set frame empty

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Set page value to undefined chars

   void VMC_PageFrame ::
             SetPageValueUndefined( )
   {

      memset( pageValue , VALUE_UNDEFINED , TAL_PageSize  ) ;
      changeLevel = TAL_CHANGED ;

   } // End of function: VMF !Set page value to undefined chars

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Set segment identification

   void VMC_PageFrame ::
             SetIdSeg( int idSeg )
   {


      idSegment = idSeg ;

   } // End of function: VMF !Set segment identification

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Set page identification

   void VMC_PageFrame ::
             SetIdPag( int idPagParm )
   {


      idPage = idPagParm ;

   } // End of function: VMF !Set page identification

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Set frame dirty

   void VMC_PageFrame :: SetFrameDirty( TAL_tpChangeLevel level )
   {


      if ( SEG_SegmentRoot::GetRoot( )->GetSegmentOpeningMode( idSegment ) ==
                TAL_OpeningModeRead )
      {
         if ( level == TAL_IGNORABLE_CHANGE )
         {
            return ;
         } /* if */
         MSG_Message * pMsg = new MSG_Message( VMC_ErrorReadOnly ) ;
         pMsg->AddItem( 0 , new MSG_ItemInteger( idPage )) ;
         pMsg->AddItem( 1 , new SEG_ItemSegmentFullName( idSegment )) ;
         EXC_USAGE( pMsg , -1 , TAL_NullIdHelp ) ;
      } /* if */

      if ( changeLevel > level )
      {
         changeLevel = level ;
      } /* if */

   } // End of function: VMF !Set frame dirty

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Set page data

   void VMC_PageFrame ::
             SetPageData( const int offset ,
                          const int length ,
                          void  * pData    ,
                          TAL_tpChangeLevel level )
   {



      memcpy( pageValue + offset , pData , length ) ;

      SetFrameDirty( level ) ;

   } // End of function: VMF !Set page data

////////////////////////////////////////////////////////////////////////////
// 
// Method: WMF !Get page data

   void VMC_PageFrame ::
             GetPageData( const int offset ,
                          const int length ,
                          void  * pData    )
   {



      memcpy( pData , pageValue + offset , length ) ;

   } // End of function: WMF !Get page data

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Get page value pointer

   char * VMC_PageFrame ::
             GetPageValue( )
   {


      if ( idSegment >= 0 )
      {
         return reinterpret_cast< char * >( pageValue ) ;
      } /* if */

      return NULL ;

   } // End of function: VMF !Get page value pointer

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Get segment id

   int VMC_PageFrame :: GetIdSeg( )
   {

      return idSegment ;

   } // End of function: VMF !Get segment id

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Get page id

   int VMC_PageFrame ::
             GetIdPag( )
   {

      return idPage ;

   } // End of function: VMF !Get page id

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Get page frame element index

   int VMC_PageFrame ::
             GetInxPageFrameElem( )
   {

      return inxPageFrameElem ;

   } // End of function: VMF !Get page frame element index

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Get number of pins of the frame

   int VMC_PageFrame :: GetNumPins( )
   {

      return numPins ;

   } // End of function: VMF !Get number of pins of the frame

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Get dirty flag

   TAL_tpChangeLevel VMC_PageFrame ::
             GetDirtyFlag( )
   {

      return changeLevel ;

   } // End of function: VMF !Get dirty flag

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Get segment file name

   STR_String * VMC_PageFrame ::
             GetSegmentFileName( )
   {

      if ( idSegment >= 0 )
      {
         return SEG_SegmentRoot::GetRoot( )->GetSegmentFileName( idSegment ) ;
      } /* if */

      return new STR_String( VMC_NullSegment ) ;

   } // End of function: VMF !Get segment file name

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMF !Get segment full name

   STR_String * VMC_PageFrame ::
                GetSegmentFullName( )
   {


      if ( idSegment >= 0 )
      {
         return SEG_SegmentRoot::GetRoot( )->GetSegmentFullName( idSegment ) ;
      } /* if */

      return new STR_String( VMC_NullSegment ) ;

   } // End of function: VMF !Get segment full name

//--- End of class: VMF  Page frame


//==========================================================================
//----- Class implementation -----
//==========================================================================

////////////////////////////////////////////////////////////////////////////
// 
// Implementation of class: VMR  Virtual memory root singleton
////////////////////////////////////////////////////////////////////////////

//==========================================================================
//----- Public method implementations -----
//==========================================================================

// Class: VMR  Virtual memory root singleton

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !:Virtual memory root create

   void VMC_VirtualMemoryRoot ::
             CreateRoot( int minFrames ,
                         int maxFrames  )
   {


      pVirtualMemoryRoot = new VMC_VirtualMemoryRoot( minFrames , maxFrames ) ;

      if ( pVirtualMemoryRoot == NULL )
      {
         MSG_Message * pMsg = new MSG_Message( VMC_ErrorOpening ) ;
         EXC_PROGRAM( pMsg , -1 , TAL_NullIdHelp ) ;
      } /* if */

   } // End of function: VMR !:Virtual memory root create

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !:Virtual memory root delete

   void VMC_VirtualMemoryRoot ::
             DestroyRoot( )
   {

      SEG_SegmentRoot * pSegRoot = SEG_SegmentRoot::GetRoot( ) ;

      if ( pSegRoot != NULL )
      {
         int idS = TAL_NullIdSeg ;

         idS = pSegRoot->GetNextIdSegment( idS ) ;

         while ( idS != TAL_NullIdSeg )
         {
            VMC_VirtualMemoryRoot::GetRoot( )->RemoveSegment( idS ) ;
            pSegRoot->CloseSegment( idS ) ;
            idS = pSegRoot->GetNextIdSegment( idS ) ;
         } /* while */

         SEG_SegmentRoot::DestroyRoot( ) ;

      } /* if */

      delete pVirtualMemoryRoot ;
      pVirtualMemoryRoot = NULL ;

   } // End of function: VMR !:Virtual memory root delete

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Is page in memory

   bool VMC_VirtualMemoryRoot ::
             IsPageInMemory( int idSeg ,
                             int idPag  )
   {


      VMC_PageFrameElement * pPageFrameElem = SearchRealPage( idSeg , idPag ) ;
      if ( pPageFrameElem != NULL )
      {
         return true ;
      } /* if */

      pPageFrameElem = lruListTail ;
      if ( pPageFrameElem->frameType == FRAME_TYPE_FREE )
      {
         ReplacePage( pPageFrameElem , idSeg , idPag , false ) ;
         totalAccessCounter ++ ;

         return true ;
      } /* if */

      return false ;

   } // End of function: VMR !Is page in memory

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Verify virtual memory root object

   #define   ASSERT_VER( Condition , idMsg )   \
      if ( !( Condition ) )                    \
      {                                        \
         if ( verifyMode == TAL_VerifyLog )    \
         {                                     \
            EXC_LOG( envelope.pMsg , idMsg ) ; \
            numErrors ++ ;                     \
         } else                                \
         {                                     \
            return 1 ;                         \
         } /* if */                            \
      }  /* if */

   int VMC_VirtualMemoryRoot ::
             VerifyVirtualMemory( TAL_tpVerifyMode verifyMode )
   {


      VMC_PageFrameElement * pPageFrameElem = NULL ;

      int numErrors = 0 ;

      // Virtual memory verification setup

         struct PointerEnvelope
         {
            MSG_Message * pMsg ;

            PointerEnvelope( )
            {
               pMsg = NULL ;
            }

           ~PointerEnvelope( )
            {
               delete pMsg ;
            }
         } envelope ; /* struct */

         if ( verifyMode == TAL_VerifyLog )
         {
            envelope.pMsg = new MSG_Message( VMC_ErrorRootVerify ) ;
         } /* if */

      // Verify virtual memory and segment singletons

         ASSERT_VER( this != NULL , 1 ) ;
         ASSERT_VER( this == pVirtualMemoryRoot , 2 ) ;
         ASSERT_VER( SEG_SegmentRoot::GetRoot( ) != NULL , 3 ) ;

         if ( numErrors != 0 )
         {
            return numErrors ;
         } /* if */

         ASSERT_VER( VerifyCorrectOpenPageCount( ) == 0 , 34 ) ;

      // Verify hash table and colision lists

         if ( envelope.pMsg != NULL )
         {
           delete envelope.pMsg ;
           envelope.pMsg = new MSG_Message( VMC_ErrorRootElemVerify ) ;
         } /* if */

         for ( int inxHash = 0 ; inxHash < TAL_dimColision ; inxHash++ )
         {
            pPageFrameElem = vtColision[ inxHash ] ;
            while ( pPageFrameElem != NULL )
            {
               if ( envelope.pMsg != NULL )
               {
                  envelope.pMsg->AddItem( 1 , new MSG_ItemInteger(
                            pPageFrameElem->inxFrameElement )) ;
               } /* if */

               ASSERT_VER( pPageFrameElem->frameType == FRAME_TYPE_IN_USE , 4 ) ;

               ASSERT_VER( pPageFrameElem->inxHash   == inxHash , 5 ) ;
               int idSeg = pPageFrameElem->pPageFrame->GetIdSeg( ) ;
               int idPag = pPageFrameElem->pPageFrame->GetIdPag( ) ;
               ASSERT_VER( idSeg >= 0 , 6 ) ;
               ASSERT_VER( idPag >= 0 , 7 ) ;
               if ( ( idSeg >= 0 ) && ( idPag >= 0 ))
               {
                  ASSERT_VER( inxHash == ComputeInxHash( idSeg , idPag ) , 8 ) ;
               } /* if */

               if ( pPageFrameElem->nextColisionElem != NULL )
               {
                  ASSERT_VER( pPageFrameElem->nextColisionElem->
                            prevColisionElem == pPageFrameElem , 9 ) ;
               } /* if */
               if ( pPageFrameElem->prevColisionElem != NULL )
               {
                  ASSERT_VER( pPageFrameElem->prevColisionElem->
                            nextColisionElem == pPageFrameElem , 10 ) ;
               } /* if */

               pPageFrameElem = pPageFrameElem->nextColisionElem ;
            } /* while */
         } /* for */

      // Verify LRU list anchors

         if ( envelope.pMsg != NULL )
         {
            envelope.pMsg->AddItem( 1 , new MSG_ItemInteger( -1 )) ;
         } /* if */

         ASSERT_VER( numPageFrames >= numMinFrames , 11 ) ;

         if ( lruListHead != NULL )
         {
            ASSERT_VER( lruListHead->prevLruElem == NULL  , 12 ) ;
         } else
         {
            ASSERT_VER( lruListHead != NULL , 13 ) ;
            return false ;
         } /* if */

         if ( lruListTail != NULL )
         {
            ASSERT_VER( lruListTail->nextLruElem == NULL , 14 ) ;
         } else
         {
            ASSERT_VER( lruListTail != NULL , 15 ) ;
            return false ;
         } /* if */

      // Verify all LRU list page frames

         pPageFrameElem = lruListHead ;
         while ( pPageFrameElem != NULL )
         {
            if ( envelope.pMsg != NULL )
            {
               envelope.pMsg->AddItem( 1 , new MSG_ItemInteger(
                         pPageFrameElem->inxFrameElement )) ;
            } /* if */
            if ( pPageFrameElem->nextLruElem != NULL )
            {
               ASSERT_VER( pPageFrameElem->nextLruElem->prevLruElem ==
                         pPageFrameElem , 16 ) ;
            } else
            {
               ASSERT_VER( pPageFrameElem == lruListTail , 18 ) ;
            } /* if */

            if ( pPageFrameElem->prevLruElem != NULL )
            {
               ASSERT_VER( pPageFrameElem->prevLruElem->nextLruElem ==
                         pPageFrameElem , 20 ) ;
            } else
            {
               ASSERT_VER( pPageFrameElem == lruListHead , 22 ) ;
            } /* if */

            if ( pPageFrameElem->frameType == FRAME_TYPE_IN_USE )
            {
               ASSERT_VER( pPageFrameElem->inxHash >= 0 , 24 ) ;
               ASSERT_VER( pPageFrameElem->inxHash < TAL_dimColision , 25 ) ;
               ASSERT_VER( vtColision[ pPageFrameElem->inxHash ] != NULL , 26 ) ;
            } else
            {
               ASSERT_VER( pPageFrameElem->frameType == FRAME_TYPE_FREE , 27 ) ;
               ASSERT_VER( pPageFrameElem->inxHash <  0 , 28 ) ;
               ASSERT_VER( pPageFrameElem->pPageFrame->GetIdSeg( ) < 0 , 29 ) ;
               ASSERT_VER( pPageFrameElem->pPageFrame->GetIdPag( ) < 0 , 30 ) ;
            } /* if */

            ASSERT_VER( pPageFrameElem->pPageFrame->GetInxPageFrameElem( ) ==
                      pPageFrameElem->inxFrameElement , 31 ) ;

            ASSERT_VER( pPageFrameElem->pPageFrame->VerifyPageFrame(
                      verifyMode ) == 0 , 32 )

            pPageFrameElem = pPageFrameElem->nextLruElem ;
         } /* while */

      ASSERT_VER( VerifyCorrectOpenPageCount( ) == 0 , 35 ) ;

      return numErrors ;

   } // End of function: VMR !Verify virtual memory root object

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Verify number of open pages

   int VMC_VirtualMemoryRoot ::
             CountOpenPages( int idSeg )
   {

      VMC_PageFrameElement * pPageFrameElem = lruListHead ;

      SEG_SegmentRoot::GetRoot( )->StartOpenPageCounter( idSeg ) ;

      while ( pPageFrameElem != NULL )
      {
         if ( pPageFrameElem->pPageFrame->GetIdSeg( ) == idSeg )
         {
            SEG_SegmentRoot::GetRoot( )->CountOpenPage( idSeg ) ;
         } /* if */

         pPageFrameElem = pPageFrameElem->nextLruElem ;
      } /* while */

      return SEG_SegmentRoot::GetRoot( )->GetOpenPageCounter( idSeg ) ;

   } // End of function: VMR !Verify number of open pages

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Verify all open pages

   int VMC_VirtualMemoryRoot ::
             VerifyOpenPages( TAL_tpVerifyMode verifyMode )
   {

      SEG_SegmentRoot::GetRoot( )->StartAllCounters( ) ;
      VMC_PageFrameElement * pPageFrameElem = lruListHead ;

      while ( pPageFrameElem != NULL )
      {
         int idSeg = pPageFrameElem->pPageFrame->GetIdSeg( ) ;
         if ( idSeg >= 0 )
         {
            SEG_SegmentRoot::GetRoot( )->CountOpenPage( idSeg ) ;
         } /* if */
         pPageFrameElem = pPageFrameElem->nextLruElem ;

      } /* while */

      return SEG_SegmentRoot::GetRoot( )->
                VerifyOpenPageCounters( verifyMode )  ;

   } // End of function: VMR !Verify all open pages

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Display virtual memory usage statistics

   void VMC_VirtualMemoryRoot ::
             DisplayStatistics( )
   {

      int numUsedFrames     = 0 ;
      int countPinned       = 0 ;

      int minColisionSize  = 32767 ;
      int maxColisionSize  = 0 ;
      int sumColisionSize  = 0 ;
      int numColisionLists = 0 ;
      int countColisions   = 0 ;

      // Gather statistics about all frames in use

         LOG_Logger * pLogger = GLB_GetGlobal( )->GetEventLogger( ) ;
         pLogger->Log( "" ) ;
         pLogger->Log( STR_GetStringAddress( VMC_FormatStatTitle )) ;

         for ( int inxHash = 0 ; inxHash < TAL_dimColision ; inxHash++ ) {

         // Gather statistics of a colision list

            countColisions = 0 ;
            VMC_PageFrameElement * pPageFrameElem = vtColision[ inxHash ] ;

            while ( pPageFrameElem != NULL )
            {
               countColisions ++ ;
               numUsedFrames   ++ ;

               if ( pPageFrameElem->pPageFrame->GetNumPins( ) > 0 )
               {
                  countPinned ++ ;
               } /* if */
               pPageFrameElem = pPageFrameElem->nextColisionElem ;
            } /* while */

         // Compute min, max and mean colision list size.

            if ( countColisions > 0 )
            {
               numColisionLists ++ ;

               if ( minColisionSize > countColisions )
               {
                  minColisionSize =   countColisions ;
               } /* if */
               if ( maxColisionSize < countColisions )
               {
                  maxColisionSize =   countColisions ;
               } /* if */
               sumColisionSize += countColisions ;
            } /* if */

         } // end repetition: Gather statistics about all frames in use

      // Print statistics

         char msg[ 100 ] ;
         sprintf( msg , STR_GetStringAddress( VMC_FormatStatPins ) ,
                 TAL_PageSize , numPageFrames , numUsedFrames , countPinned , maxPinnedFrames ) ;
         pLogger->Log( msg ) ;

         double hitRate = totalHitCounter ;
         hitRate = hitRate * 100.0 / totalAccessCounter ;

         sprintf( msg , STR_GetStringAddress( VMC_FormatStatAccess ) ,
                 totalAccessCounter , totalReplaceCounter , totalHitCounter ,
                 hitRate ) ;

         pLogger->Log( msg ) ;

         sprintf( msg , STR_GetStringAddress( VMC_FormatStatTotals ) ,
                 SEG_SegmentRoot::GetRoot( )->GetTotalPagesRead( ) ,
                 SEG_SegmentRoot::GetRoot( )->GetTotalPagesWritten( ) ,
                 SEG_SegmentRoot::GetRoot( )->GetTotalPagesAdded( ) ) ;

         pLogger->Log( msg ) ;
         if ( numColisionLists > 0 )
         {
            double sumCol = sumColisionSize ;
            sprintf( msg , STR_GetStringAddress( VMC_FormatStatColl ) ,
                    minColisionSize , maxColisionSize ,
                    sumCol / numColisionLists ) ;
            pLogger->Log( msg ) ;
         } /* if */
         pLogger->Log( "" ) ;

   } // End of function: VMR !Display virtual memory usage statistics

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Display list of pinned frames

   void VMC_VirtualMemoryRoot ::
             DisplayPinnedFrameList( )
   {

      LOG_Logger * pLogger = GLB_GetGlobal( )->GetEventLogger( ) ;
      pLogger->Log( STR_GetStringAddress( VMC_FormatPinList )) ;

      const int dimMsg = 50 ;
      char msg[ dimMsg ] ;

      const int dimBuffer = 25 ;
      char  buffer[ dimBuffer ] ;

      int  count = 0 ;
      VMC_PageFrameElement * pPageFrameElem = lruListHead ;

      while ( pPageFrameElem != NULL )
      {
         if ( pPageFrameElem->pPageFrame->GetNumPins( ) != 0 )
         {
            if ( count % 2 == 0 )
            {
               pLogger->Log( "    " ) ;
            } /* if */

            STR_String * pSegName = pPageFrameElem->pPageFrame->
                      GetSegmentFileName( ) ;
            STR_ConvertToPrintable( pSegName->GetLength( ) , pSegName->GetString( ) ,
                      dimBuffer - 5 , buffer , false ) ;
            delete pSegName ;
            sprintf( msg , STR_GetStringAddress( VMC_FormatPinElem ) , buffer ,
                      pPageFrameElem->pPageFrame->GetIdPag( ) ,
                      pPageFrameElem->pPageFrame->GetNumPins( ) ) ;

            pLogger->Log( msg , false ) ;
            count ++ ;
         } /* if */
         pPageFrameElem = pPageFrameElem->nextLruElem ;
      } /* while */
      if ( count == 0 )
      {
         pLogger->Log( STR_GetStringAddress( VMC_FormatPinEmpty )) ;
      } /* if */
      pLogger->Log( "" ) ;

   } // End of function: VMR !Display list of pinned frames

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Write all dirty frames

   void VMC_VirtualMemoryRoot ::
             WriteAllPageFrames( )
   {

      VMC_PageFrameElement * pPageFrameElem = lruListHead ;

      while ( pPageFrameElem != NULL )
      {
         pPageFrameElem->pPageFrame->WritePageFrame( ) ;
         pPageFrameElem = pPageFrameElem->nextLruElem ;
      } /* while */

   } // End of function: VMR !Write all dirty frames

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Remove all pages of a given segment

   void VMC_VirtualMemoryRoot ::
             RemoveSegment( int idSeg )
   {

      if ( idSeg < 0 )
      {
         return ;
      } /* if */

      VMC_PageFrameElement * pPageFrameElem    = lruListHead ;
      VMC_PageFrameElement * nextPageFrameElem = NULL ;

      while ( pPageFrameElem != NULL )
      {
         nextPageFrameElem = pPageFrameElem->nextLruElem ;
         if ( pPageFrameElem->pPageFrame->GetIdSeg( ) == idSeg )
         {
            RemovePageValue( pPageFrameElem ) ;
            MoveElemLruTail( pPageFrameElem ) ;
         } /* if */
         pPageFrameElem = nextPageFrameElem ;
      } /* while */

   } // End of function: VMR !Remove all pages of a given segment

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Add new page value to end of segment file

   VMC_PageFrame * VMC_VirtualMemoryRoot ::
             AddNewPage( int idSeg )
   {

      SEG_SegmentRoot * pRoot = SEG_SegmentRoot::GetRoot( ) ;
      int idPag = pRoot->GetSegmentNumPages( idSeg ) ;

      VMC_PageFrameElement * pPageFrameElem = GetEmptyFrame( idSeg , idPag ) ;
      pRoot->AddPage( idSeg , pPageFrameElem->pPageFrame->GetPageValue( )) ;

      return pPageFrameElem->pPageFrame ;

   } // End of function: VMR !Add new page value to end of segment file

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Get LRU list head

   VMC_PageFrameElement * VMC_VirtualMemoryRoot ::
             GetLruListHead( )
   {

      return lruListHead ;

   } // End of function: VMR !Get LRU list head

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Get next LRU list element

   VMC_PageFrameElement * VMC_VirtualMemoryRoot ::
             GetNextLruListElement( VMC_PageFrameElement * currentElem )
   {


      return currentElem->nextLruElem ;

   } // End of function: VMR !Get next LRU list element

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Get page frame

   VMC_PageFrame * VMC_VirtualMemoryRoot ::
             GetPageFrame( int idSeg ,
                           int idPag ,
                           bool inMemory )
   {


      // Access frame already in memory

         VMC_PageFrameElement * pPageFrameElem = SearchRealPage( idSeg , idPag ) ;

         if ( pPageFrameElem != NULL )
         {
            totalAccessCounter ++ ;
            totalHitCounter ++ ;
            if ( !inMemory )
            {
               MoveElemLruHead( pPageFrameElem ) ;
            } /* if */

            return pPageFrameElem->pPageFrame ;
         } /* if */

      // Replace empty frame

         pPageFrameElem = lruListTail ;
         if ( pPageFrameElem->frameType == FRAME_TYPE_FREE )
         {
            ReplacePage( pPageFrameElem , idSeg , idPag , false ) ;
            totalAccessCounter ++ ;

            return pPageFrameElem->pPageFrame ;
         } /* if */

      // Replace unpinned frame

         if ( inMemory )
         {
            return NULL ;
         } /* if */

         pPageFrameElem = FindReplaceableFrame( ) ;
         ReplacePage( pPageFrameElem , idSeg , idPag , false ) ;
         totalAccessCounter ++ ;

         return pPageFrameElem->pPageFrame ;

   } // End of function: VMR !Get page frame

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Get number page frames

   int VMC_VirtualMemoryRoot ::
             GetNumPageFrames( )
   {

      return numPageFrames ;

   } // End of function: VMR !Get number page frames

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Get total frame accesses

   int VMC_VirtualMemoryRoot :: GetTotalAccesses( )
   {

      return totalAccessCounter ;

   } // End of function: VMR !Get total frame accesses

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Get total frame replaces

   int VMC_VirtualMemoryRoot ::
             GetTotalReplaces( )
   {

      return totalReplaceCounter ;

   } // End of function: VMR !Get total frame replaces

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Get total hit count

   int VMC_VirtualMemoryRoot ::
             GetTotalHits( )
   {

      return totalHitCounter ;

   } // End of function: VMR !Get total hit count

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Get number of pinned frames

   int VMC_VirtualMemoryRoot ::
            GetNumPinnedFrames( )
   {

      int countPinned = 0 ;
      VMC_PageFrameElement * pPageFrameElem = lruListHead ;

      while ( pPageFrameElem != NULL )
      {
         if ( pPageFrameElem->pPageFrame->GetNumPins( ) != 0 )
         {
            countPinned ++ ;
         } /* if */
         pPageFrameElem = pPageFrameElem->nextLruElem ;
      } /* while */

      return countPinned ;

   } // End of function: VMR !Get number of pinned frames

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR !Get page frame of current element

   VMC_PageFrame * VMC_VirtualMemoryRoot ::
             GetPageFrame( VMC_PageFrameElement * currentElem )
   {


      return currentElem->pPageFrame ;

   } // End of function: VMR !Get page frame of current element

//==========================================================================
//----- Protected method implementations -----
//==========================================================================

// Class: VMR  Virtual memory root singleton

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR #Virtual memory root constructor

   VMC_VirtualMemoryRoot ::
             VMC_VirtualMemoryRoot( int minFramesParm ,
                                    int maxFramesParm  )
   {

      StartUpVirtualMemory( minFramesParm , maxFramesParm ) ;

   } // End of function: VMR #Virtual memory root constructor

////////////////////////////////////////////////////////////////////////////
// 
// Method: VMR #Virtual memory root destructor

   VMC_VirtualMemoryRoot :: ~VMC_VirtualMemoryRoot( )
   {

      VMC_PageFrameElement * nextPageFrameElem = NULL ;
      VMC_PageFrameElement * pPageFrameElem    = lruListHead ;
      while ( pPageFrameElem != NULL )
      {
         nextPageFrameElem = pPageFrameElem->nextLruElem ;
         delete pPageFrameElem ;
         pPageFrameElem = nextPageFrameElem ;
      } /* while */

      SEG_SegmentRoot::DestroyRoot( ) ;

   } // End of function: VMR #Virtual memory root destructor

//==========================================================================
//----- Private method implementations -----
//==========================================================================

// Class: VMR  Virtual memory root singleton

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Search page frame element of virtual page already in real memory
//    Returns the pointer to the page frame element that contains the
//    virtual page
//    NULL if not found
// 
////////////////////////////////////////////////////////////////////////////

   VMC_PageFrameElement *  VMC_VirtualMemoryRoot ::
             SearchRealPage( int idSeg ,
                             int idPag  )
   {

      int inxHash = ComputeInxHash( idSeg , idPag ) ;

      VMC_PageFrameElement * pPageFrameElem = vtColision[ inxHash ] ;

      while ( pPageFrameElem != NULL )
      {
         if ( ( pPageFrameElem->pPageFrame->GetIdSeg( ) == idSeg )
           && ( pPageFrameElem->pPageFrame->GetIdPag( ) == idPag ))
         {
            return pPageFrameElem ;
         } /* if */
         pPageFrameElem = pPageFrameElem->nextColisionElem ;
      } /* while */

      return NULL ;

   } // End of function: VMR $Search page frame element of virtual page already in real memory

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Start up virtual memory
//    Creates the LRU and write-page list heads
//    All detected errors throw an exception
// 
// Parameters
//    $P numPageFrames  - numeber of frames to be allocated
// 
// Returned exceptions
//    Failure if the minimum number of frames cannot be allocated.
// 
////////////////////////////////////////////////////////////////////////////

   void VMC_VirtualMemoryRoot ::
             StartUpVirtualMemory( int minFramesParm ,
                                   int maxFramesParm  )
   {

      // Clear counters

         totalHitCounter     = 0 ;
         totalAccessCounter  = 0 ;
         totalReplaceCounter = 0 ;

      // Allocate page frames

         VMC_PageFrameElement * pPageFrameElem ;

         lruListHead = NULL ;
         lruListTail = NULL ;

         int countFrames  ;
         for ( countFrames = 0 ; countFrames < maxFramesParm ; countFrames++ )
         {
            try
            {
               pPageFrameElem = new VMC_PageFrameElement( countFrames ) ;
               if ( pPageFrameElem == NULL )        // instrumentation catch
               {
                  break ;
               } /* if */

               pPageFrameElem->frameType   = FRAME_TYPE_FREE ;

               if ( countFrames == 0 )
               {
                  lruListTail = pPageFrameElem ;
               } else
               {
                  lruListHead->prevLruElem = pPageFrameElem ;
               } /* if */

               pPageFrameElem->nextLruElem = lruListHead ;
               lruListHead                 = pPageFrameElem ;

            } // end try
            catch( ... )
            {
               break ;          // out of memory
            } // end try catch
         }

         numPageFrames = countFrames ;

         if( numPageFrames < minFramesParm )
         {
            MSG_Message * pMsg = new MSG_Message( VMC_InsufficientFrames ) ;
            pMsg->AddItem( 0 , new MSG_ItemInteger( numPageFrames )) ;
            EXC_USAGE( pMsg , -1 , TAL_NullIdHelp ) ;
         }

      // Clean hash vector

         for ( int i = 0 ; i < TAL_dimColision ; i++ )
         {
            vtColision[ i ] = NULL ;
         } /* for */

      // Create the segment root singleton

         SEG_SegmentRoot::CreateRoot( ) ;

   } // End of function: VMR $Start up virtual memory

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Find a replaceable page frame element
//    Searches for a non pinned frame from tail to head of LRU
// 
// Return value
//    Pointer to a replaceable page frame found.
// 
////////////////////////////////////////////////////////////////////////////

   VMC_PageFrameElement * VMC_VirtualMemoryRoot ::
             FindReplaceableFrame( )
   {

      VMC_PageFrameElement * pPageFrameElem = lruListTail ;

      while ( pPageFrameElem != NULL )
      {
         if ( pPageFrameElem->pPageFrame->GetNumPins( ) == 0 )
         {
            return pPageFrameElem ;
         } /* if */
         pPageFrameElem = pPageFrameElem->prevLruElem ;
      } /* while */

      MSG_Message * pMsg = new MSG_Message( VMC_NoFreeFrame ) ;
      EXC_PROGRAM( pMsg , -1 , TAL_NullIdHelp ) ;

   } // End of function: VMR $Find a replaceable page frame element

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Move to LRU head the page frame element
//    pPageFrameElem - element to be moved
// 
////////////////////////////////////////////////////////////////////////////

   void VMC_VirtualMemoryRoot ::
             MoveElemLruHead( VMC_PageFrameElement * pPageFrameElem )
   {


      if ( pPageFrameElem->prevLruElem != NULL )
      {

         pPageFrameElem->prevLruElem->nextLruElem =
                   pPageFrameElem->nextLruElem ;
         if ( pPageFrameElem->nextLruElem != NULL )
         {
            pPageFrameElem->nextLruElem->prevLruElem =
                      pPageFrameElem->prevLruElem ;
         } else
         {
            lruListTail = pPageFrameElem->prevLruElem ;
         } /* if */

         pPageFrameElem->prevLruElem = NULL ;
         pPageFrameElem->nextLruElem = lruListHead ;
         lruListHead->prevLruElem = pPageFrameElem ;
         lruListHead = pPageFrameElem ;

      } // end selection: Root of VMR $Move to LRU head the page frame element

   } // End of function: VMR $Move to LRU head the page frame element

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Move to LRU tail the page frame element
// 
////////////////////////////////////////////////////////////////////////////

   void VMC_VirtualMemoryRoot ::
             MoveElemLruTail( VMC_PageFrameElement * pPageFrameElem )
   {

      if ( pPageFrameElem->nextLruElem != NULL )
      {

         pPageFrameElem->nextLruElem->prevLruElem =
                   pPageFrameElem->prevLruElem ;
         if ( pPageFrameElem->prevLruElem != NULL )
         {
            pPageFrameElem->prevLruElem->nextLruElem =
                      pPageFrameElem->nextLruElem ;
         } else
         {
            lruListHead = pPageFrameElem->nextLruElem ;
         } /* if */

         pPageFrameElem->nextLruElem = NULL ;
         pPageFrameElem->prevLruElem = lruListTail ;
         lruListTail->nextLruElem    = pPageFrameElem ;
         lruListTail = pPageFrameElem ;

      } // end selection: Root of VMR $Move to LRU tail the page frame element

   } // End of function: VMR $Move to LRU tail the page frame element

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Replace page in frame
//    Replaces the page contained in the frame by another one read from
//    a given segment.
// 
// Parameters
//    isNewPage - true if page is not to be read, but set to undefined chars
//                false if should be read
// 
////////////////////////////////////////////////////////////////////////////

   void VMC_VirtualMemoryRoot ::
             ReplacePage( VMC_PageFrameElement * pPageFrameElem ,
                          int  idSeg    ,
                          int  idPag    ,
                          bool isNewPage )
   {

      totalReplaceCounter ++ ;

      RemovePageValue( pPageFrameElem ) ;

      if ( isNewPage )
      {
         pPageFrameElem->pPageFrame->SetIdSeg( idSeg ) ;
         pPageFrameElem->pPageFrame->SetIdPag( idPag ) ;
      } else

      {
         try
         {
            pPageFrameElem->pPageFrame->ReadPageFrame( idSeg , idPag ) ;
         } // end try
         catch( ... )
         {
            pPageFrameElem->pPageFrame->SetFrameEmpty( ) ;
            pPageFrameElem->frameType = FRAME_TYPE_FREE ;

            throw ;

         } // end try catch
      } /* if */

      pPageFrameElem->frameType = FRAME_TYPE_IN_USE ;
      SEG_SegmentRoot::GetRoot( )->GetSegment( idSeg )->
                 IncreaseNumOpenPages( ) ;

      LinkColisionList( pPageFrameElem ) ;
      MoveElemLruHead(  pPageFrameElem ) ;

   } // End of function: VMR $Replace page in frame

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Get empty page frame
//    Gets a frame without reading it from disk.
//    This function is used when a new page must be added to the segment.
//    The page value contained in the frame is set to undefined chars.
// 
// Returned exceptions
//    Enforce - if the page to be added already exists in real memory
// 
////////////////////////////////////////////////////////////////////////////

   VMC_PageFrameElement * VMC_VirtualMemoryRoot ::
             GetEmptyFrame( int idSeg ,
                            int idPag  )
   {


      VMC_PageFrameElement * pPageFrameElem = SearchRealPage( idSeg , idPag ) ;

      pPageFrameElem = FindReplaceableFrame( ) ;
      ReplacePage( pPageFrameElem , idSeg , idPag , true ) ;

      return pPageFrameElem ;

   } // End of function: VMR $Get empty page frame

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Remove page from frame
//    Removes the page from the page frame.
//    After this operation the frame is empty.
//    The LRU list is not changed.
// 
////////////////////////////////////////////////////////////////////////////

   void VMC_VirtualMemoryRoot ::
             RemovePageValue( VMC_PageFrameElement * pPageFrameElem )
   {

      if ( pPageFrameElem->frameType == FRAME_TYPE_IN_USE )
      {
         pPageFrameElem->pPageFrame->WritePageFrame( ) ;

         UnlinkColisionList( pPageFrameElem ) ;

         SEG_SegmentRoot::GetRoot( )->GetSegment( pPageFrameElem->
                   pPageFrame->GetIdSeg( ))->DecreaseNumOpenPages( ) ;
      } /* if */

      pPageFrameElem->pPageFrame->SetFrameEmpty( ) ;
      pPageFrameElem->frameType = FRAME_TYPE_FREE ;

   } // End of function: VMR $Remove page from frame

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Compute hash index
//    Computes the hash index of a virtual address.
//    This index is used by the hash table that searches for pages
//    already i memory.
// 
////////////////////////////////////////////////////////////////////////////

   int VMC_VirtualMemoryRoot ::
             ComputeInxHash( int idSeg , int idPag )
   {


      return static_cast< int >
                (( idSeg * 2083 + idPag ) % TAL_dimColision ) ;

   } // End of function: VMR $Compute hash index

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Link page frame into colision list
// 
////////////////////////////////////////////////////////////////////////////

   void VMC_VirtualMemoryRoot ::
             LinkColisionList( VMC_PageFrameElement * pPageFrameElem )
   {

      int idSeg = pPageFrameElem->pPageFrame->GetIdSeg( ) ;
      int idPag = pPageFrameElem->pPageFrame->GetIdPag( ) ;
      pPageFrameElem->inxHash = ComputeInxHash( idSeg , idPag ) ;

      pPageFrameElem->nextColisionElem = vtColision[ pPageFrameElem->inxHash ] ;
      pPageFrameElem->prevColisionElem = NULL ;
      vtColision[ pPageFrameElem->inxHash ] = pPageFrameElem ;

      if ( pPageFrameElem->nextColisionElem != NULL )
      {
         pPageFrameElem->nextColisionElem->prevColisionElem = pPageFrameElem ;
      } /* if */

   } // End of function: VMR $Link page frame into colision list

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Unlink page frame from colision list
// 
////////////////////////////////////////////////////////////////////////////

   void VMC_VirtualMemoryRoot ::
             UnlinkColisionList( VMC_PageFrameElement * pPageFrameElem )
   {


      if ( pPageFrameElem->inxHash >= 0 )
      {

         if ( pPageFrameElem->nextColisionElem != NULL )
         {
            pPageFrameElem->nextColisionElem->prevColisionElem =
                      pPageFrameElem->prevColisionElem ;
         } /* if */
         if ( pPageFrameElem->prevColisionElem != NULL )
         {
            pPageFrameElem->prevColisionElem->nextColisionElem =
                      pPageFrameElem->nextColisionElem ;
         } else
         {
            vtColision[ pPageFrameElem->inxHash ] =
                      pPageFrameElem->nextColisionElem ;
         } /* if */

         pPageFrameElem->nextColisionElem = NULL ;
         pPageFrameElem->prevColisionElem = NULL ;
         pPageFrameElem->inxHash = -1 ;

      } // end selection: Root of VMR $Unlink page frame from colision list

   } // End of function: VMR $Unlink page frame from colision list

////////////////////////////////////////////////////////////////////////////
// 
//  Method: VMR $Verify and correct all page counters
//    This method assures that the number of open pages is equal to the
//    actual instantaneous segment by segment count of open pages.
// 
////////////////////////////////////////////////////////////////////////////

   int VMC_VirtualMemoryRoot ::
             VerifyCorrectOpenPageCount( )
   {

      if ( VerifyOpenPages( TAL_VerifyLog ) != 0 )
      {
         SEG_SegmentRoot::GetRoot( )->ResetOpenPages( ) ;
         return 1 ;
      } /* if */

      return 0 ;

   } // End of function: VMR $Verify and correct all page counters

//--- End of class: VMR  Virtual memory root singleton

////// End of implementation module: VMC  VRTMEM Virtual memory control ////

