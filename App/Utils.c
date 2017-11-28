#include <App/Global.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//*****************************************************************************
//
//!    prints the formatted string on to the console
//!
//! \param format is a pointer to the character string specifying the format in
//!           the following arguments need to be interpreted.
//! \param [variable number of] arguments according to the format in the first
//!         parameters
//! This function
//!        1. prints the formatted error statement.
//!
//! \return count of characters printed
//
//*****************************************************************************
void Log (const char *pcFormat, ...)
{
  int iRet = 0;

  char *pcBuff, *pcTemp;
  int iSize = 256;

  va_list list;
  pcBuff = (char*)malloc(iSize);
  if(pcBuff == NULL)
  {
      return;
  }
  while(1)
  {
      va_start(list,pcFormat);
      iRet = vsnprintf(pcBuff,iSize,pcFormat,list);
      va_end(list);
      if(iRet > -1 && iRet < iSize)
      {
          break;
      }
      else
      {
          iSize*=2;
          if((pcTemp=realloc(pcBuff,iSize))==NULL)
          {
              UART_write(uart, "Could not reallocate memory\n\r", strlen("Could not reallocate memory\n\r"));
              iRet = -1;
              break;
          }
          else
          {
              pcBuff=pcTemp;
          }

      }
  }
  UART_write(uart, pcBuff, strlen (pcBuff));
  UART_write(uart, "\n\r", 2);

  free(pcBuff);
}



