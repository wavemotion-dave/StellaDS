   // Calculate the ending frame pointer value
   uInt8* ending = myFramePointer + clocksToUpdate;
   
   // --------------------------------------------------------------------------------
   // Ball and Playfield (very common) this is all stuff that starts with 0x30..0x3F
   // --------------------------------------------------------------------------------
   if ((myEnabledObjects & (myPFBit | myBLBit)) == (myPFBit | myBLBit)) 
   {
       if ((myEnabledObjects == (myPFBit | myBLBit)) && ((!(myPlayfieldPriorityAndScore & ScoreBit)))) // Playfield and Ball only are enabled
       {
             uInt32* mPF = &myCurrentPFMask[hpos];
             uInt8* mBL = &myCurrentBLMask[hpos];

             while(myFramePointer < ending)
             {
                 if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL)
                 {
                     *(uInt32*)myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                     mPF += 4; mBL += 4; myFramePointer += 4;
                 }
                 else
                 {
                     *myFramePointer = ((myPF & *mPF) || *mBL) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
#ifndef COLLISIONS_OFF
                     if((myPF & *mPF) && *mBL) myCollision |= ourCollisionTable[myPFBit | myBLBit];
#endif                     
                     ++mPF; ++mBL; ++myFramePointer;
                 }
             }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myP1Bit | myP0Bit)) // Playfield and Ball plus Player 1 and Player 0 enabled...
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myP0Bit)) // Playfield and Ball plus Player 0 enabled...
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myM0Bit)) // Playfield and Ball plus Missile 0 enabled... (Elevators Amiss)
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myM1Bit)) // Playfield and Ball plus Missile 1 enabled...
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(*mM1++)                        enabled |= myM1Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myP1Bit | myM0Bit)) // Playfield and Ball plus Player 1 and Missile 0 enabled...
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myM1Bit | myP1Bit)) // Playfield and Ball plus Missle 1 and Player 1
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM1++)                        enabled |= myM1Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myP0Bit | myM0Bit | myM1Bit)) // Playfield and Ball plus Player 0 and Missile 0 plus Missile 1 enabled...
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           if(*mM1++)                        enabled |= myM1Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myM1Bit | myP1Bit | myM0Bit)) 
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM1++)                        enabled |= myM1Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }       
       else if (myEnabledObjects == (myPFBit | myBLBit | myP1Bit | myM0Bit | myP0Bit)) // Playfield and Ball plus P1, M0 and P0
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }       
       else if (myEnabledObjects == (myPFBit | myBLBit | myP0Bit | myM0Bit)) // Playfield and Ball plus Player 0 and Missile 0 enabled...
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myP1Bit)) // Playfield and Ball plus Player 1 enabled...
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myM0Bit | myM1Bit)) // Playfield and Ball plus Missile 0 and Missile 1 enabled...
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(*mM0++)                        enabled |= myM0Bit;
           if(*mM1++)                        enabled |= myM1Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myM1Bit | myP0Bit)) // Playfield and Ball plus Missle 1 and Player 0
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(*mM1++)                        enabled |= myM1Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myBLBit | myM1Bit | myP1Bit | myM0Bit | myP0Bit)) // Everything is enbaled...
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM1++)                        enabled |= myM1Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }       
       else  // Unsure... need to check them all... this is slow.
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         uInt8 meo1 = (myEnabledObjects & myBLBit) ? 0xFF:0x00;
         uInt8 meo2 = (myEnabledObjects & myM1Bit) ? 0xFF:0x00;
         uInt8 meo3 = (myEnabledObjects & myM0Bit) ? 0xFF:0x00;
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(meo1 & *mBL++)                 enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(meo2 & *mM1++)                 enabled |= myM1Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(meo3 & *mM0++)                 enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }  
   }
   // -------------------------------------------------------------------------
   // Player 1 and Player 0 are both set (fairly common)
   // -------------------------------------------------------------------------
   else if ((myEnabledObjects & (myP0Bit | myP1Bit)) == (myP0Bit | myP1Bit)) 
   {
       if (myEnabledObjects == (myP0Bit | myP1Bit)) // Player 0 and 1 is enabled only...
       {
             uInt8* mP0 = &myCurrentP0Mask[hpos];
             uInt8* mP1 = &myCurrentP1Mask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0 && !*(uInt32*)mP1)
               {
                 *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                 mP0 += 4; mP1 += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = (myCurrentGRP0 & *mP0) ? 
                     myColor[MYCOLUP0] : ((myCurrentGRP1 & *mP1) ? myColor[MYCOLUP1] : myColor[MYCOLUBK]);

#ifndef COLLISIONS_OFF
                 if((myCurrentGRP0 & *mP0) && (myCurrentGRP1 & *mP1))
                   myCollision |= ourCollisionTable[myP0Bit | myP1Bit];
#endif
                 ++mP0; ++mP1; ++myFramePointer;
               }
             }
       }
       else if (myEnabledObjects == (myP0Bit | myP1Bit | myPFBit))   // Player 0 and 1 plus Playfield
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myP0Bit | myP1Bit | myPFBit | myM1Bit)) // Player 0 and 1 plus Playfield plus Missile 1
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM1++)                        enabled |= myM1Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }          
       else if (myEnabledObjects == (myP0Bit | myP1Bit | myBLBit | myM1Bit)) // Player 0 and 1 plus Ball plus Missile 1
       {
         
         
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];              
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = myPlayfieldPriorityAndScore;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM1++)                        enabled |= myM1Bit;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }          
       else if (myEnabledObjects == (myP0Bit | myP1Bit | myM1Bit)) // Player 0 and 1 plus Missile 1
       {
         
         
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = myPlayfieldPriorityAndScore;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM1++)                        enabled |= myM1Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }          
       else if (myEnabledObjects == (myP0Bit | myP1Bit | myPFBit | myM0Bit)) // // Player 0 and 1 plus Playfield plus Missile 0
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }  
       else if (myEnabledObjects == (myP0Bit | myP1Bit | myM0Bit | myBLBit | myPFBit)) // // Player 0 and 1 plus Playfield plus Missile 0 plus Ball
       {
             
             
             uInt32*mPF = &myCurrentPFMask[hpos];
             uInt8* mP1 = &myCurrentP1Mask[hpos];
             uInt8* mP0 = &myCurrentP0Mask[hpos];
             uInt8* mM0 = &myCurrentM0Mask[hpos];
             uInt8* mBL = &myCurrentBLMask[hpos];
             for(; myFramePointer < ending; ++myFramePointer)
             {
               uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
               if(*mBL++)                        enabled |= myBLBit;
               if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
               if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
               if(*mM0++)                        enabled |= myM0Bit;
               HANDLE_COLOR_AND_COLLISIONS;
             }
       }
       else if (myEnabledObjects == (myP0Bit | myP1Bit | myM0Bit | myM1Bit | myBLBit)) // Player 0/1 plus Missile 0/1 plus Ball enabled...
       {
         
         
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM1++)                        enabled |= myM1Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myP0Bit | myP1Bit | myBLBit)) // // Player 0 and 1 plus Ball
       {
             
             
             uInt8* mP1 = &myCurrentP1Mask[hpos];
             uInt8* mP0 = &myCurrentP0Mask[hpos];
             uInt8* mBL = &myCurrentBLMask[hpos];
             for(; myFramePointer < ending; ++myFramePointer)
             {
               uInt8 enabled = myPlayfieldPriorityAndScore;
               if(*mBL++)                        enabled |= myBLBit;
               if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
               if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
               HANDLE_COLOR_AND_COLLISIONS;
             }
       }
       else if (myEnabledObjects == (myP0Bit | myP1Bit | myPFBit | myM0Bit | myM1Bit)) // // Playfield plus M0, M1, P0, P1
       {
             
             
             uInt32*mPF = &myCurrentPFMask[hpos];
             uInt8* mP0 = &myCurrentP0Mask[hpos];
             uInt8* mP1 = &myCurrentP1Mask[hpos];
             uInt8* mM0 = &myCurrentM0Mask[hpos];
             uInt8* mM1 = &myCurrentM1Mask[hpos];
             for(; myFramePointer < ending; ++myFramePointer)
             {
               uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
               if(*mM0++)                        enabled |= myM0Bit;
               if(*mM1++)                        enabled |= myM1Bit;
               if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
               if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
               HANDLE_COLOR_AND_COLLISIONS;
             }
       }
       else if (myEnabledObjects == (myP0Bit | myM0Bit | myP1Bit)) // Player 0 and Missile 0 and Missile 1 are enabled
       {
         
         
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];            
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = myPlayfieldPriorityAndScore;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }          
       else // Unsure... need to check them all... this is slow.
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         uInt8 meo1 = (myEnabledObjects & myBLBit) ? 0xFF:0x00;
         uInt8 meo2 = (myEnabledObjects & myM1Bit) ? 0xFF:0x00;
         uInt8 meo3 = (myEnabledObjects & myM0Bit) ? 0xFF:0x00;
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(meo1 & *mBL++)                 enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(meo2 & *mM1++)                 enabled |= myM1Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(meo3 & *mM0++)                 enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
   }
   // -------------------------------------------------------------------------
   // Playfield Bit is set... (fairly common)
   // -------------------------------------------------------------------------
   else if ((myEnabledObjects & (myPFBit)) == myPFBit) // Playfield is Set (fairly common)
   {
       if (myEnabledObjects == myPFBit) // Playfield bit set... with or without score
       {
           uInt32* mask = &myCurrentPFMask[hpos];
           if (myPlayfieldPriorityAndScore & PriorityBit)   // Priority bit overrides the Score bit
           {
             // Update a uInt8 at a time until reaching a uInt32 boundary
             for(; ((uintptr_t)myFramePointer & 0x03); ++myFramePointer, ++mask)
             {
               *myFramePointer = (myPF & *mask) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
             }

             // Now, update a uInt32 at a time
             for(; myFramePointer < ending; myFramePointer += 4, mask += 4)
             {
               *((uInt32*)myFramePointer) = (myPF & *mask) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
             }
           }
           else if (myPlayfieldPriorityAndScore & ScoreBit)   // Playfield is enabled and the score bit is set 
           {
             // Update a uInt8 at a time until reaching a uInt32 boundary
             for(; ((uintptr_t)myFramePointer & 0x03); ++myFramePointer, ++mask, ++hpos)
             {
               *myFramePointer = (myPF & *mask) ? (hpos < 80 ? myColor[MYCOLUP0] : myColor[MYCOLUP1]) : myColor[MYCOLUBK];
             }

             // Now, update a uInt32 at a time
             for(; myFramePointer < ending;  myFramePointer += 4, mask += 4, hpos += 4)
             {
               *((uInt32*)myFramePointer) = (myPF & *mask) ? (hpos < 80 ? myColor[MYCOLUP0] : myColor[MYCOLUP1]) : myColor[MYCOLUBK];
             }
           }          
           else  // Playfield is enabled and the score bit is not set and priority clear...
           {
             // Update a uInt8 at a time until reaching a uInt32 boundary
             for(; ((uintptr_t)myFramePointer & 0x03); ++myFramePointer, ++mask)
             {
               *myFramePointer = (myPF & *mask) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
             }

             // Now, update a uInt32 at a time
             for(; myFramePointer < ending; myFramePointer += 4, mask += 4)
             {
               *((uInt32*)myFramePointer) = (myPF & *mask) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
             }
           }
       }      
       else if (myEnabledObjects == (myM0Bit | myPFBit)) // Playfield + Missile 0
       {
             
             
             uInt32*mPF = &myCurrentPFMask[hpos];
             uInt8* mM0 = &myCurrentM0Mask[hpos];
             uInt8 meo3 = (myEnabledObjects & myM0Bit) ? 0xFF:0x00;
             for(; myFramePointer < ending; ++myFramePointer)
             {
               uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
               if(meo3 & *mM0++)                enabled |= myM0Bit;
               HANDLE_COLOR_AND_COLLISIONS;
             }
       }
       else if (myEnabledObjects == (myPFBit | myP0Bit)) // Playfield and Player 0 are enabled 
       {
           if (myPlayfieldPriorityAndScore & PriorityBit) // Priority set
           {
             uInt32* mPF = &myCurrentPFMask[hpos];
             uInt8* mP0 = &myCurrentP0Mask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0)
               {
                 *(uInt32*)myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                 mPF += 4; mP0 += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : 
                     ((myCurrentGRP0 & *mP0) ? myColor[MYCOLUP0] : myColor[MYCOLUBK]);
#ifndef COLLISIONS_OFF
                 if((myPF & *mPF) && (myCurrentGRP0 & *mP0))
                   myCollision |= ourCollisionTable[myPFBit | myP0Bit];
#endif
                 ++mPF; ++mP0; ++myFramePointer;
               }
             }
           }
           else if (!(myPlayfieldPriorityAndScore & ScoreBit)) // Priority not set and Score not set
           {
             uInt32* mPF = &myCurrentPFMask[hpos];
             uInt8* mP0 = &myCurrentP0Mask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0)
               {
                 *(uInt32*)myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                 mPF += 4; mP0 += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = (myCurrentGRP0 & *mP0) ? 
                       myColor[MYCOLUP0] : ((myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK]);
#ifndef COLLISIONS_OFF
                 if((myPF & *mPF) && (myCurrentGRP0 & *mP0))
                   myCollision |= ourCollisionTable[myPFBit | myP0Bit];
#endif
                 ++mPF; ++mP0; ++myFramePointer;
               }
             }
           }
           else // Need to do this the hard way...
           {
             
             
             uInt32*mPF = &myCurrentPFMask[hpos];
             uInt8* mP0 = &myCurrentP0Mask[hpos];
             for(; myFramePointer < ending; ++myFramePointer)
             {
               uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
               if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
               HANDLE_COLOR_AND_COLLISIONS;
             }
           }
       }
       else if (myEnabledObjects == (myPFBit | myP1Bit)) // Playfield and Player 1 are enabled 
       {
           if (myPlayfieldPriorityAndScore & PriorityBit) // Priority set
           {
             uInt32* mPF = &myCurrentPFMask[hpos];
             uInt8* mP1 = &myCurrentP1Mask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1)
               {
                 *(uInt32*)myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                 mPF += 4; mP1 += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : 
                     ((myCurrentGRP1 & *mP1) ? myColor[MYCOLUP1] : myColor[MYCOLUBK]);
#ifndef COLLISIONS_OFF
                 if((myPF & *mPF) && (myCurrentGRP1 & *mP1))
                   myCollision |= ourCollisionTable[myPFBit | myP1Bit];
#endif
                 ++mPF; ++mP1; ++myFramePointer;
               }
             }
           }
           else if (!(myPlayfieldPriorityAndScore & ScoreBit)) // Priority not set and Score not set
           {
             uInt32* mPF = &myCurrentPFMask[hpos];
             uInt8* mP1 = &myCurrentP1Mask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1)
               {
                 *(uInt32*)myFramePointer = (myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                 mPF += 4; mP1 += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = (myCurrentGRP1 & *mP1) ? 
                       myColor[MYCOLUP1] : ((myPF & *mPF) ? myColor[MYCOLUPF] : myColor[MYCOLUBK]);
#ifndef COLLISIONS_OFF
                 if((myPF & *mPF) && (myCurrentGRP1 & *mP1))
                   myCollision |= ourCollisionTable[myPFBit | myP1Bit];
#endif
                 ++mPF; ++mP1; ++myFramePointer;
               }
             }
           }
           else // Need to do this the hard way...
           {
             
             
             uInt32*mPF = &myCurrentPFMask[hpos];
             uInt8* mP1 = &myCurrentP1Mask[hpos];
             for(; myFramePointer < ending; ++myFramePointer)
             {
               uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
               if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
               HANDLE_COLOR_AND_COLLISIONS;
             }
           }
       }          
       else if (myEnabledObjects == (myPFBit | myP1Bit | myM0Bit)) // Playfield and Player 1 and Missile 0 are enabled 
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }      
       else if (myEnabledObjects == (myPFBit | myP0Bit | myM0Bit)) // Playfield and Player 0 and Missile 0 are enabled 
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myM1Bit | myM0Bit)) // Playfield, Missile 1, Missile 0
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mM1++)                        enabled |= myM1Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myM1Bit)) // Playfield, Missile 1
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mM1++)                        enabled |= myM1Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myM1Bit | myP1Bit)) // Playfield, Missile 1, Player 1
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mM1++)                        enabled |= myM1Bit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myM1Bit | myM0Bit | myP0Bit)) // Playfield, Missile 0, Missile 1, Player 1
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mM1++)                        enabled |= myM1Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myPFBit | myM1Bit | myM0Bit | myP1Bit)) // Playfield, Missile 0, Missile 1, Player 1
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(*mM1++)                        enabled |= myM1Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else // Catch-all
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         uInt8 meo1 = (myEnabledObjects & myBLBit) ? 0xFF:0x00;
         uInt8 meo2 = (myEnabledObjects & myM1Bit) ? 0xFF:0x00;
         uInt8 meo3 = (myEnabledObjects & myM0Bit) ? 0xFF:0x00;
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(meo1 & *mBL++)                enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(meo2 & *mM1++)                enabled |= myM1Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(meo3 & *mM0++)                enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       } 
   }
   // -------------------------------------------------------------------------
   // Ball is set (fairly common) - but not playfield
   // -------------------------------------------------------------------------
   else if ((myEnabledObjects & (myBLBit)) == myBLBit) // Ball is set (fairly common)
   {
       if (myEnabledObjects == myBLBit) // Ball is enabled (Official Frogger)
       {
             uInt8* mBL = &myCurrentBLMask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL)
               {
                 *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                 mBL += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = *mBL ? myColor[MYCOLUPF] : myColor[MYCOLUBK];
                 ++mBL; ++myFramePointer;
               }
             }
       }          
       else if (myEnabledObjects == (myBLBit | myP0Bit)) // Ball + Player 0 (freeway)
       {
             
             
             uInt8* mP0 = &myCurrentP0Mask[hpos];
             uInt8* mBL = &myCurrentBLMask[hpos];
             for(; myFramePointer < ending; ++myFramePointer)
             {
               uInt8 enabled = myPlayfieldPriorityAndScore;
               if(*mBL++)                        enabled |= myBLBit;
               if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
               HANDLE_COLOR_AND_COLLISIONS;
             }
       }
       else if (myEnabledObjects == (myBLBit | myP0Bit | myM0Bit)) // Ball plus Player 0 and Missile 0 are enabled
       {
         
         
         uInt8* mBL = &myCurrentBLMask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = myPlayfieldPriorityAndScore;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(*mM0++)                        enabled |= myM0Bit;
           if(*mBL++)                        enabled |= myBLBit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }
       else if (myEnabledObjects == (myBLBit | myM0Bit)) // Ball and Missle 0 are enabled
       {
           if (myPlayfieldPriorityAndScore & PriorityBit) // Priority set
           {
             uInt8* mBL = &myCurrentBLMask[hpos];
             uInt8* mM0 = &myCurrentM0Mask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM0)
               {
                 *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                 mBL += 4; mM0 += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = (*mBL ? myColor[MYCOLUPF] : (*mM0 ? myColor[MYCOLUP0] : myColor[MYCOLUBK]));
#ifndef COLLISIONS_OFF
                 if(*mBL && *mM0)
                   myCollision |= ourCollisionTable[myBLBit | myM0Bit];
#endif
                 ++mBL; ++mM0; ++myFramePointer;
               }
             }
           }
           else if (!(myPlayfieldPriorityAndScore & ScoreBit)) // Priority not set and Score not set
           {
             uInt8* mBL = &myCurrentBLMask[hpos];
             uInt8* mM0 = &myCurrentM0Mask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM0)
               {
                 *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                 mBL += 4; mM0 += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = (*mM0 ? myColor[MYCOLUP0] : (*mBL ? myColor[MYCOLUPF] : myColor[MYCOLUBK]));
#ifndef COLLISIONS_OFF
                 if(*mBL && *mM0)
                   myCollision |= ourCollisionTable[myBLBit | myM0Bit];
#endif
                 ++mBL; ++mM0; ++myFramePointer;
               }
             }
           }
           else // Need to do this the hard way...
           {
             
             
             uInt32*mPF = &myCurrentPFMask[hpos];
             uInt8* mM0 = &myCurrentM0Mask[hpos];
             uInt8* mBL = &myCurrentBLMask[hpos];
             uInt8 meo1 = (myEnabledObjects & myBLBit) ? 0xFF:0x00;
             uInt8 meo2 = (myEnabledObjects & myM0Bit) ? 0xFF:0x00;
             for(; myFramePointer < ending; ++myFramePointer)
             {
               uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
               if(meo1 & *mBL++)                enabled |= myBLBit;
               if(meo2 & *mM0++)                enabled |= myM0Bit;
               HANDLE_COLOR_AND_COLLISIONS;
             }
           }
       }
       else if (myEnabledObjects == (myBLBit | myM1Bit)) // Ball and Missle 1 are enabled
       {
           if (myPlayfieldPriorityAndScore & PriorityBit) // Priority set
           {
             uInt8* mBL = &myCurrentBLMask[hpos];
             uInt8* mM1 = &myCurrentM1Mask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM1)
               {
                 *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                 mBL += 4; mM1 += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = (*mBL ? myColor[MYCOLUPF] : (*mM1 ? myColor[MYCOLUP1] : myColor[MYCOLUBK]));
#ifndef COLLISIONS_OFF
                 if(*mBL && *mM1)
                   myCollision |= ourCollisionTable[myBLBit | myM1Bit];
#endif
                 ++mBL; ++mM1; ++myFramePointer;
               }
             }
           }
           else if (!(myPlayfieldPriorityAndScore & ScoreBit)) // Priority not set and Score not set
           {
             uInt8* mBL = &myCurrentBLMask[hpos];
             uInt8* mM1 = &myCurrentM1Mask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mBL && !*(uInt32*)mM1)
               {
                 *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                 mBL += 4; mM1 += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = (*mM1 ? myColor[MYCOLUP1] : (*mBL ? myColor[MYCOLUPF] : myColor[MYCOLUBK]));
#ifndef COLLISIONS_OFF
                 if(*mBL && *mM1)
                   myCollision |= ourCollisionTable[myBLBit | myM1Bit];
#endif
                 ++mBL; ++mM1; ++myFramePointer;
               }
             }
           }
           else // Need to do this the hard way...
           {
             
             
             uInt32*mPF = &myCurrentPFMask[hpos];
             uInt8* mM1 = &myCurrentM1Mask[hpos];
             uInt8* mBL = &myCurrentBLMask[hpos];
             uInt8 meo1 = (myEnabledObjects & myBLBit) ? 0xFF:0x00;
             uInt8 meo2 = (myEnabledObjects & myM1Bit) ? 0xFF:0x00;
             for(; myFramePointer < ending; ++myFramePointer)
             {
               uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
               if(meo1 & *mBL++)                enabled |= myBLBit;
               if(meo2 & *mM1++)                enabled |= myM1Bit;
               HANDLE_COLOR_AND_COLLISIONS;
             }
           }
       }
       else if (myEnabledObjects == (myBLBit | myP1Bit)) // Ball and Player 1 are enabled
       {
           if (myPlayfieldPriorityAndScore & PriorityBit) // Priority set
           {
             uInt8* mBL = &myCurrentBLMask[hpos];
             uInt8* mP1 = &myCurrentP1Mask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1 && !*(uInt32*)mBL)
               {
                 *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                 mBL += 4; mP1 += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = *mBL ? myColor[MYCOLUPF] : 
                     ((myCurrentGRP1 & *mP1) ? myColor[MYCOLUP1] : myColor[MYCOLUBK]);
#ifndef COLLISIONS_OFF
                 if(*mBL && (myCurrentGRP1 & *mP1))
                   myCollision |= ourCollisionTable[myBLBit | myP1Bit];
#endif
                 ++mBL; ++mP1; ++myFramePointer;
               }
             }
           }
           else if (!(myPlayfieldPriorityAndScore & ScoreBit)) // Priority not set and Score not set
           {
             uInt8* mBL = &myCurrentBLMask[hpos];
             uInt8* mP1 = &myCurrentP1Mask[hpos];

             while(myFramePointer < ending)
             {
               if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1 && !*(uInt32*)mBL)
               {
                 *(uInt32*)myFramePointer = myColor[MYCOLUBK];
                 mBL += 4; mP1 += 4; myFramePointer += 4;
               }
               else
               {
                 *myFramePointer = (myCurrentGRP1 & *mP1) ? myColor[MYCOLUP1] : 
                     (*mBL ? myColor[MYCOLUPF] : myColor[MYCOLUBK]);
#ifndef COLLISIONS_OFF
                 if(*mBL && (myCurrentGRP1 & *mP1))
                   myCollision |= ourCollisionTable[myBLBit | myP1Bit];
#endif
                 ++mBL; ++mP1; ++myFramePointer;
               }
             }
           }
           else // Need to do this the hard way...
           {
             
             
             uInt32*mPF = &myCurrentPFMask[hpos];
             uInt8* mP1 = &myCurrentP1Mask[hpos];
             uInt8* mBL = &myCurrentBLMask[hpos];
             uInt8 meo1 = (myEnabledObjects & myBLBit) ? 0xFF:0x00;
             for(; myFramePointer < ending; ++myFramePointer)
             {
               uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
               if(meo1 & *mBL++)                enabled |= myBLBit;
               if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
               HANDLE_COLOR_AND_COLLISIONS;
             }
           }
       }     
       else if (myEnabledObjects == (myBLBit | myP1Bit | myM1Bit)) // Ball, Player 1, Missile 1
       {
         
         
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = myPlayfieldPriorityAndScore;
           if(*mBL++)                        enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(*mM1++)                        enabled |= myM1Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }          
       else if (myEnabledObjects == (myBLBit | myM0Bit | myM1Bit)) // Ball + Missile 0/1
       {
         
         
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = myPlayfieldPriorityAndScore;
           if(*mBL++)                enabled |= myBLBit;
           if(*mM1++)                enabled |= myM1Bit;
           if(*mM0++)                enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       }          
       else // Catch-all
       {
         
         
         uInt32*mPF = &myCurrentPFMask[hpos];
         uInt8* mP1 = &myCurrentP1Mask[hpos];
         uInt8* mP0 = &myCurrentP0Mask[hpos];
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];
         uInt8* mBL = &myCurrentBLMask[hpos];
         uInt8 meo1 = (myEnabledObjects & myBLBit) ? 0xFF:0x00;
         uInt8 meo2 = (myEnabledObjects & myM1Bit) ? 0xFF:0x00;
         uInt8 meo3 = (myEnabledObjects & myM0Bit) ? 0xFF:0x00;
         for(; myFramePointer < ending; ++myFramePointer)
         {
           uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
           if(meo1 & *mBL++)                enabled |= myBLBit;
           if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
           if(meo2 & *mM1++)                enabled |= myM1Bit;
           if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
           if(meo3 & *mM0++)                enabled |= myM0Bit;
           HANDLE_COLOR_AND_COLLISIONS;
         }
       } 
   }
   // -------------------------------------------------------------------------
   // Otherwise we do the rest of the simple compares below... 
   // -------------------------------------------------------------------------
   else if (myEnabledObjects == myP0Bit) // Player 0 is enabled
   {
         uInt8* mP0 = &myCurrentP0Mask[hpos];

         while(myFramePointer < ending)
         {
           if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP0)
           {
             *(uInt32*)myFramePointer = myColor[MYCOLUBK];
             mP0 += 4; myFramePointer += 4;
           }
           else
           {
             *myFramePointer = (myCurrentGRP0 & *mP0) ? myColor[MYCOLUP0] : myColor[MYCOLUBK];
             ++mP0; ++myFramePointer;
           }
         }
   }
   else if (myEnabledObjects == myP1Bit) // Player 1 is enabled
   {
         uInt8* mP1 = &myCurrentP1Mask[hpos];

         while(myFramePointer < ending)
         {
           if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mP1)
           {
             *(uInt32*)myFramePointer = myColor[MYCOLUBK];
             mP1 += 4; myFramePointer += 4;
           }
           else
           {
             *myFramePointer = (myCurrentGRP1 & *mP1) ? myColor[MYCOLUP1] : myColor[MYCOLUBK];
             ++mP1; ++myFramePointer;
           }
         }
   }
   else if (myEnabledObjects == myM0Bit) // Missile 0 is enabled
   {
         uInt8* mM0 = &myCurrentM0Mask[hpos];

         while(myFramePointer < ending)
         {
           if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mM0)
           {
             *(uInt32*)myFramePointer = myColor[MYCOLUBK];
             mM0 += 4; myFramePointer += 4;
           }
           else
           {
             *myFramePointer = *mM0 ? myColor[MYCOLUP0] : myColor[MYCOLUBK];
             ++mM0; ++myFramePointer;
           }
         }
   }
   else if (myEnabledObjects == myM1Bit) // Missile 1 is enabled
   {
         uInt8* mM1 = &myCurrentM1Mask[hpos];

         while(myFramePointer < ending)
         {
           if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mM1)
           {
             *(uInt32*)myFramePointer = myColor[MYCOLUBK];
             mM1 += 4; myFramePointer += 4;
           }
           else
           {
             *myFramePointer = *mM1 ? myColor[MYCOLUP1] : myColor[MYCOLUBK];
             ++mM1; ++myFramePointer;
           }
         }
   }
   else if (myEnabledObjects == (myP0Bit | myM0Bit)) // Player 0 and Missile 0 are enabled (Super Breakout!!)
   {
     
     
     uInt8* mP0 = &myCurrentP0Mask[hpos];
     uInt8* mM0 = &myCurrentM0Mask[hpos];
     for(; myFramePointer < ending; ++myFramePointer)
     {
       uInt8 enabled = myPlayfieldPriorityAndScore;
       if(myCurrentGRP0 & *mP0++)        enabled |= myP0Bit;
       if(*mM0++)                        enabled |= myM0Bit;
       HANDLE_COLOR_AND_COLLISIONS;
     }
   }
   else if (myEnabledObjects == (myM0Bit | myM1Bit)) // Missile 0 and 1 is enabled
   {
         uInt8* mM0 = &myCurrentM0Mask[hpos];
         uInt8* mM1 = &myCurrentM1Mask[hpos];

         while(myFramePointer < ending)
         {
           if(!((uintptr_t)myFramePointer & 0x03) && !*(uInt32*)mM0 && !*(uInt32*)mM1)
           {
             *(uInt32*)myFramePointer = myColor[MYCOLUBK];
             mM0 += 4; mM1 += 4; myFramePointer += 4;
           }
           else
           {
             *myFramePointer = *mM0 ? myColor[MYCOLUP0] : (*mM1 ? myColor[MYCOLUP1] : myColor[MYCOLUBK]);
#ifndef COLLISIONS_OFF
             if(*mM0 && *mM1)
               myCollision |= ourCollisionTable[myM0Bit | myM1Bit];
#endif
             ++mM0; ++mM1; ++myFramePointer;
           }
         }
   }      
   else if (myEnabledObjects == (myP1Bit | myM1Bit)) // Player 1, Missile 1
   {
     
     
     uInt8* mP1 = &myCurrentP1Mask[hpos];
     uInt8* mM1 = &myCurrentM1Mask[hpos];
     for(; myFramePointer < ending; ++myFramePointer)
     {
       uInt8 enabled = myPlayfieldPriorityAndScore;
       if(myCurrentGRP1 & *mP1++)        enabled |= myP1Bit;
       if(*mM1++)                        enabled |= myM1Bit;
       HANDLE_COLOR_AND_COLLISIONS;
     }
   }
   else // Catch-all... this is a bit slow so do this only if no matches above...
   {
     
     
     uInt32*mPF = &myCurrentPFMask[hpos];
     uInt8* mP1 = &myCurrentP1Mask[hpos];
     uInt8* mP0 = &myCurrentP0Mask[hpos];
     uInt8* mM0 = &myCurrentM0Mask[hpos];
     uInt8* mM1 = &myCurrentM1Mask[hpos];
     uInt8* mBL = &myCurrentBLMask[hpos];
     uInt8 meo1 = (myEnabledObjects & myBLBit) ? 0xFF:0x00;
     uInt8 meo2 = (myEnabledObjects & myM1Bit) ? 0xFF:0x00;
     uInt8 meo3 = (myEnabledObjects & myM0Bit) ? 0xFF:0x00;
     for(; myFramePointer < ending; ++myFramePointer)
     {
       uInt8 enabled = (myPF & *mPF++) ? (myPFBit| myPlayfieldPriorityAndScore) : myPlayfieldPriorityAndScore;
       if(meo1 & *mBL++)                enabled |= myBLBit;
       if(myCurrentGRP1 & *mP1++)       enabled |= myP1Bit;
       if(meo2 & *mM1++)                enabled |= myM1Bit;
       if(myCurrentGRP0 & *mP0++)       enabled |= myP0Bit;
       if(meo3 & *mM0++)                enabled |= myM0Bit;
       HANDLE_COLOR_AND_COLLISIONS;
     }
   }        
   myFramePointer = ending;
