#include <windows.h>
#include <stdio.h>
#include "ff.h"
#include "diskio.h"


#define MIN_FAT16	4086U	/* Minimum number of clusters for FAT16 */
#define	MIN_FAT32	65526U	/* Minimum number of clusters for FAT32 */

#define BPB_NumFATs			16		/* Number of FAT copies (1) */
#define BPB_RootEntCnt		17		/* Number of root directory entries for FAT12/16 (2) */
#define BPB_TotSec16		19		/* Volume size [sector] (2) */
#define BPB_FATSz16			22		/* FAT size [sector] (2) */
#define BPB_TotSec32		32		/* Volume size [sector] (4) */
#define BPB_FATSz32			36		/* FAT size [sector] (4) */

extern get_fat(FATFS*, DWORD);	    /* Read an FAT item (FatFs hidden API) */


BYTE *RamDisk;		                /* Poiter to the RAM disk */
DWORD RamDiskSize;	                /* Size of RAM disk in unit of sector */


static FATFS  FatFs;
static FIL    DstFile;
static HANDLE SrcFile;
static TCHAR  src_path[512], DstPath[512];
static BYTE   Buff[4096];
static UINT   Dirs, Files;

static WIN32_FIND_DATA Fd;



int maketree (void)
{
	HANDLE  hdir;
	int     slen, dlen, rv = 0;
	DWORD   br;
	UINT    bw;

	slen = wcslen(src_path);
	dlen = wcslen(DstPath);

    wsprintf(&src_path[slen], L"/*");

	hdir = FindFirstFile(src_path, &Fd);		/* Open directory */
	if (hdir == INVALID_HANDLE_VALUE) 
    {
		wprintf(L"Failed to open directory.\n");
	} 
    else 
    {
		for (;;) 
        {
            wsprintf(&src_path[slen], L"/%s", Fd.cFileName);
            wsprintf(&DstPath[dlen],  L"/%s", Fd.cFileName);


			if (Fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)    /* The item is a directory */
            {	
				if (wcscmp(Fd.cFileName, L".") && wcscmp(Fd.cFileName, L".."))
                {
					if (f_mkdir(DstPath))    /* Create destination directory */
                    {	
						printf("Failed to create directory.\n"); break;
					}

					if (!maketree())    /* Enter the directory */
                        break;	

					Dirs++;
				}
			} 
            else    /* The item is a file */
            {	
                wprintf(L"%s\n", src_path);
				if ((SrcFile = CreateFile(src_path, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)   /* Open source file */
                {	
                    wprintf(L"Failed to open source file.\n"); 
                    break;
				}

				if (f_open(&DstFile, DstPath, FA_CREATE_ALWAYS | FA_WRITE))    /* Create destination file */
                {	
                    wprintf(L"Failed to create destination file.\n"); 
                    break;
				}

				do   /* Copy source file to destination file */
                {	
					ReadFile(SrcFile, Buff, sizeof(Buff), &br, 0);
					if (br == 0) 
                        break;

					f_write(&DstFile, Buff, (UINT)br, &bw);
				} while (br == bw);

				CloseHandle(SrcFile);
				f_close(&DstFile);

				if (br && br != bw) 
                {
					wprintf(L"Failed to write file.\n"); 
                    break;
				}
				Files++;
			}
			if (!FindNextFile(hdir, &Fd)) 
            {
				rv = 1; 
                break;
			}
		}
		FindClose(hdir);
	}
	src_path[slen] = 0;
	DstPath[dlen] = 0;
	return rv;
}

//int wmain()
//{
//	FRESULT ret;
//	DIR     dir = {0};
//	RamDiskSize = 65526+16*1024;
//	
//	ret = f_mount(&FatFs, L"", 0);
//
//	ret = f_mkfs(L"", 1, 512);
//
//	ret = f_mkdir(L"projects");
//	ret = f_mkdir(L"projects/one");
//
//	ret = f_opendir(&dir, L"projects/one");
//	f_closedir(&dir);
//}


int wmain (int argc, wchar_t* argv[])
{
	UINT    csz;
    wchar_t path[MAX_PATH];

    wcscpy(path,     L"C:\\tmp\\fat32.dat");
    wcscpy(src_path, L"D:\\tmp\\CLCL\\English");
	RamDiskSize      =  64 * 1024 * 2;
	csz              =  512;

	/* create an FAT volume */
	f_mount(&FatFs, L"", 0);

    if (f_open_fs(path))        
	    if (f_create_fs(path, 1, csz))
        {
            wprintf(L"Failed to create FAT volume. Adjust volume size or cluster size.\n");
	    	return 2;
	    }

	/* Copy directory tree onto the FAT volume */
	if (!maketree()) 
        return 3;

    f_unmount(&FatFs);

	if (!Files) 
    { 
        wprintf(L"No file in the source directory."); 
        return 3; 
    }

	return 0;
}