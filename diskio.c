#include <windows.h>
#include "diskio.h"
#include "ff.h"



struct fat_driver_info
{
	HANDLE          windows_file;
	DWORD           thread;
	size_t          offset;
    size_t          size_file;

	int             start_sector;
	char            data[1024 * 1024];
};

static struct fat_driver_info *fat_info = NULL;

/*-------------------------------------------------------------------------*/
/*																		   */
/*   Module Private Functions											   */
/*																		   */
/*-------------------------------------------------------------------------*/


SYSTEMTIME   sys_time;	    /* Time at creation of RAM disk */

extern BYTE *RamDisk;		/* Poiter to the RAM disk (main.c) */
extern DWORD RamDiskSize;	/* Size of RAM disk in unit of sector (main.c) */

/*-----------------------------------------------------------------------*/
/*																		 */
/*   Public Functions													 */
/*																		 */
/*-----------------------------------------------------------------------*/

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	LPCWSTR path				/* Path */
)
{
    DSTATUS err = STA_NOINIT;

	if ( !path) 
		return err;

    if (fat_info)
    {
        return 0;
    }

    fat_info = malloc(sizeof(*fat_info));
    if (!fat_info)
        return err;

    memset(fat_info, 0, sizeof(*fat_info));

    fat_info->windows_file = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (fat_info->windows_file == INVALID_HANDLE_VALUE)
    {
        free(fat_info);
        fat_info = NULL;
        return err;
    }

    fat_info->offset = 0;
    fat_info->size_file = GetFileSize(fat_info->windows_file, NULL);

    fat_info->thread = GetCurrentThreadId();

    return 0;
}


DSTATUS disk_shutdown()
{
    if ( !fat_info)
    {
        return 0;
    }

    CloseHandle(fat_info->windows_file);
    free(fat_info);
    fat_info = NULL;

    return 0;
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0) */
)
{
    DSTATUS err = STA_NOINIT;

	if (pdrv) 
        return err;

    if ( !fat_info)
        return err;

    if (fat_info->windows_file == INVALID_HANDLE_VALUE)
        return err;

    return 0;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE     pdrv,			/* Physical drive nmuber (0)                     */
	BYTE    *buff,			/* Pointer to the data buffer to store read data */
	DWORD    sector,		/* Start sector number (LBA)                     */
	UINT     count			/* Number of sectors to read                     */
)
{
    DWORD full_size, readed;

    if (pdrv)
        return RES_NOTRDY;

    if ( !fat_info)
        return RES_NOTRDY;

    if(fat_info->windows_file == INVALID_HANDLE_VALUE)
        return RES_NOTRDY;

    if (sector >= RamDiskSize)
        return RES_PARERR;

    full_size = sector * 512 + count * 512;

    if (full_size > fat_info->size_file)
    {
        SetFilePointer(fat_info->windows_file, full_size, NULL, FILE_BEGIN);
        fat_info->size_file = full_size;
    } 
    
    SetFilePointer(fat_info->windows_file, sector * 512, NULL, FILE_BEGIN);
    if (ReadFile(fat_info->windows_file, buff, count * 512, &readed, NULL)==FALSE)
        return RES_ERROR;

    if (readed != count * 512)
        return RES_ERROR;

    return RES_OK;	
}


/*--------------------------------------------------------------------*/
/* Write Sector(s)                                                    */
/*--------------------------------------------------------------------*/
DRESULT disk_write (
	BYTE        pdrv,	        /*  Physical drive nmuber (0)         */
	const BYTE *buff,	        /*  Pointer to the data to be written */
	DWORD       sector,	        /*  Start sector number (LBA)         */
	UINT        count	        /*  Number of sectors to write        */
)
{
    DWORD writed=0, full_size;

    if (pdrv)
        return RES_NOTRDY;

    if (!fat_info)
        return RES_NOTRDY;

    if (fat_info->windows_file == INVALID_HANDLE_VALUE)
        return RES_NOTRDY;

    if (sector >= RamDiskSize)
        return RES_PARERR;

    SetFilePointer(fat_info->windows_file, sector * 512, NULL, FILE_BEGIN);
    if (WriteFile(fat_info->windows_file, buff, count * 512, &writed, NULL) == FALSE)
        return RES_ERROR;

    if (writed != count * 512)
        return RES_ERROR;

    full_size = sector * 512 + count * 512;
    if (full_size > fat_info->size_file)
        fat_info->size_file = full_size;

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,				/* Physical drive nmuber (0) */
	BYTE ctrl,				/* Control code */
	void* buff				/* Buffer to send/receive data block */
)
{
	DRESULT err;
	
	err = RES_ERROR;
    if ( !pdrv && fat_info)
	{
		switch (ctrl) 
		{
			case CTRL_SYNC:
				err = RES_OK;
				break;

			case GET_SECTOR_COUNT:
				*(DWORD*)buff = RamDiskSize;
				err = RES_OK;
				break;

			case GET_BLOCK_SIZE:
				*(DWORD*)buff = 1;
				err = RES_OK;
				break;
		}
	}

	return err;
}


/*-----------------------------------------------------------------------*/
/* Get current time                                                      */
/*-----------------------------------------------------------------------*/
DWORD get_fattime(void)
{
	return 	  (DWORD)(sys_time.wYear - 1980) << 25
			| (DWORD)sys_time.wMonth         << 21
			| (DWORD)sys_time.wDay           << 16
			| (DWORD)sys_time.wHour          << 11
			| (DWORD)sys_time.wMinute        << 5
			| (DWORD)sys_time.wSecond        >> 1;
}
