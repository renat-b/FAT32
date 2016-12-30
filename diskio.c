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

static struct fat_driver_info *global_fat_info[16] = { 0 };
wchar_t   fat32_file_name[MAX_PATH] = { 0 };
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
struct fat_driver_info *disk_driver_find(void)
{
    HANDLE thread_id = GetCurrentThreadId();
    struct fat_driver_info *driver;

    for (int i = 0; i < _countof(global_fat_info); i++)
    {
        if (global_fat_info[i]->thread == thread_id)
        {
            return global_fat_info[i];
        }

        if (global_fat_info[i]->thread == 0)
        {
            driver = malloc(sizeof(*driver));
            if (!driver)
                return NULL;


            memset(driver, 0, sizeof(*driver));

            driver->windows_file = CreateFile(fat32_file_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

            if (driver->windows_file == INVALID_HANDLE_VALUE)
            {
                free(driver);
                driver = NULL;
                return NULL;
            }

            driver->offset = 0;
            driver->size_file = GetFileSize(driver->windows_file, NULL);

            driver->thread = GetCurrentThreadId();
            global_fat_info[i] = driver;
            return driver;
        }
    }

    return NULL;
}
/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	LPCWSTR path				/* Path */
)
{
    DSTATUS err = STA_NOINIT;
    fat_driver_info *ret = NULL;

	if ( !path) 
		return err;
   
    ret = disk_driver_find();

    if (!ret)
        return err;



    if (global_fat_info)
    {
        return 0;
    }

    global_fat_info = malloc(sizeof(*global_fat_info));
    if (!global_fat_info)
        return err;

    memset(global_fat_info, 0, sizeof(*global_fat_info));

    global_fat_info->windows_file = CreateFile(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (global_fat_info->windows_file == INVALID_HANDLE_VALUE)
    {
        free(global_fat_info);
        global_fat_info = NULL;
        return err;
    }

    global_fat_info->offset = 0;
    global_fat_info->size_file = GetFileSize(global_fat_info->windows_file, NULL);

    global_fat_info->thread = GetCurrentThreadId();

    return 0;
}

DSTATUS disk_shutdown()
{
    if ( !global_fat_info)
    {
        return 0;
    }

    CloseHandle(global_fat_info->windows_file);
    free(global_fat_info);
    global_fat_info = NULL;

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

    if ( !global_fat_info)
        return err;

    if (global_fat_info->windows_file == INVALID_HANDLE_VALUE)
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

    if ( !global_fat_info)
        return RES_NOTRDY;

    if(global_fat_info->windows_file == INVALID_HANDLE_VALUE)
        return RES_NOTRDY;

    if (sector >= RamDiskSize)
        return RES_PARERR;

    full_size = sector * 512 + count * 512;

    if (full_size > global_fat_info->size_file)
    {
        SetFilePointer(global_fat_info->windows_file, full_size, NULL, FILE_BEGIN);
        global_fat_info->size_file = full_size;
    } 
    
    SetFilePointer(global_fat_info->windows_file, sector * 512, NULL, FILE_BEGIN);
    if (ReadFile(global_fat_info->windows_file, buff, count * 512, &readed, NULL)==FALSE)
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

    if (!global_fat_info)
        return RES_NOTRDY;

    if (global_fat_info->windows_file == INVALID_HANDLE_VALUE)
        return RES_NOTRDY;

    if (sector >= RamDiskSize)
        return RES_PARERR;

    SetFilePointer(global_fat_info->windows_file, sector * 512, NULL, FILE_BEGIN);
    if (WriteFile(global_fat_info->windows_file, buff, count * 512, &writed, NULL) == FALSE)
        return RES_ERROR;

    if (writed != count * 512)
        return RES_ERROR;

    full_size = sector * 512 + count * 512;
    if (full_size > global_fat_info->size_file)
        global_fat_info->size_file = full_size;

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
    if ( !pdrv && global_fat_info)
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
