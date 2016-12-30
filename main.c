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
static TCHAR  src_path[512], dst_path[512];
static BYTE   buff[4096];
static UINT   dirs_count, files_count;
static WIN32_FIND_DATA Fd;

int maketree (void)
{
	HANDLE  hdir;
	int     slen, dlen, rv = 0;
	DWORD   br;
	UINT    bw;

	slen = wcslen(src_path);
	dlen = wcslen(dst_path);

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
            wsprintf(&dst_path[dlen],  L"/%s", Fd.cFileName);

			if (Fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)    /* The item is a directory */
            {	
				if (wcscmp(Fd.cFileName, L".") && wcscmp(Fd.cFileName, L".."))
                {
					if (f_mkdir(dst_path))       /* Create destination directory */
                    {	
						wprintf(L"Failed to create directory.\n");
                        break;
					}

					if (!maketree())            /* Enter the directory */
                        break;	

					dirs_count++;
				}
			} 
            else
            {
                wprintf(L"%s\n", src_path);
				if ((SrcFile = CreateFile(src_path, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)   /* Open source file */
                {	
                    wprintf(L"Failed to open source file.\n"); 
                    break;
				}

				if (f_open(&DstFile, dst_path, FA_CREATE_ALWAYS | FA_WRITE))    /* Create destination file */
                {	
                    wprintf(L"Failed to create destination file.\n"); 
                    break;
				}

				do   /* Copy source file to destination file */
                {	
					ReadFile(SrcFile, buff, sizeof(buff), &br, 0);
					if (br == 0) 
                        break;

					f_write(&DstFile, buff, (UINT)br, &bw);
				} while (br == bw);

				CloseHandle(SrcFile);
				f_close(&DstFile);

				if (br && br != bw) 
                {
					wprintf(L"Failed to write file.\n"); 
                    break;
				}
				files_count++;
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
	dst_path[dlen] = 0;
	return rv;
}

//int wmain()
//{	
//    UINT    csz;
//    wchar_t path[MAX_PATH];
//    DIR     dir;
//
//    wcscpy(path,     L"C:\\tmp\\fat32.dat");
//    RamDiskSize      =  64 * 1024 * 2;
//    csz              =  512;
//    
//    /* create an FAT volume */
////    f_mount(&FatFs, L"", 0);
//    
//    if (f_open_fs(path))        
//        if (f_create_fs(path, 1, csz))
//          {
//              wprintf(L"Failed to create FAT volume. Adjust volume size or cluster size.\n");
//        	  return 2;
//          }
//
//	f_opendir(&dir, L"2/2_2");
//	f_closedir(&dir);
//
//    f_unmount(&FatFs);
//}


int wmain (int argc, wchar_t* argv[])
{
	UINT    csz;
    wchar_t path[MAX_PATH];

    wcscpy(path,     L"C:\\tmp\\fat32.dat");
    wcscpy(src_path, L"D:\\tmp\\FAT32");
	RamDiskSize      =  64 * 1024 * 2;
	csz              =  512;

	/* create an FAT volume */
	f_mount(&FatFs, L"", 0);

    if (f_open_fs(path) !=0)        
	    if (f_create_fs(path, 1, csz) !=0)
        {
            wprintf(L"Failed to create FAT volume. Adjust volume size or cluster size.\n");
	    	return 2;
	    }

	/* Copy directory tree onto the FAT volume */
	if (!maketree()) 
        return 3;

    f_unmount(&FatFs);

	if (!files_count) 
    { 
        wprintf(L"No file in the source directory."); 
        return 3; 
    }

	return 0;
}