/*--------------------------------------------------------/
/  FAT image creator R0.01               (C)ChaN, 2014
/--------------------------------------------------------*/

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
static TCHAR  SrcPath[512], DstPath[512];
static BYTE   Buff[4096];
static UINT   Dirs, Files;

static WIN32_FIND_DATA Fd;



//int maketree (void)
//{
//	HANDLE  hdir;
//	int     slen, dlen, rv = 0;
//	DWORD   br;
//	UINT    bw;
//
//	slen = wcslen(SrcPath);
//	dlen = wcslen(DstPath);
//
//    wsprintf(&SrcPath[slen], L"/*");
//
//	hdir = FindFirstFile(SrcPath, &Fd);		/* Open directory */
//	if (hdir == INVALID_HANDLE_VALUE) 
//    {
//		wprintf(L"Failed to open directory.\n");
//	} 
//    else 
//    {
//		for (;;) 
//        {
//            wsprintf(&SrcPath[slen], L"/%s", Fd.cFileName);
//            wsprintf(&DstPath[dlen], L"/%s", Fd.cFileName);
//
//
//			if (Fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)    /* The item is a directory */
//            {	
//				if (wcscmp(Fd.cFileName, L".") && wcscmp(Fd.cFileName, L".."))
//                {
//					if (f_mkdir(DstPath))    /* Create destination directory */
//                    {	
//						printf("Failed to create directory.\n"); break;
//					}
//
//					if (!maketree())    /* Enter the directory */
//                        break;	
//
//					Dirs++;
//				}
//			} 
//            else    /* The item is a file */
//            {	
//                wprintf(L"%s\n", SrcPath);
//				if ((SrcFile = CreateFile(SrcPath, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0)) == INVALID_HANDLE_VALUE)   /* Open source file */
//                {	
//                    wprintf(L"Failed to open source file.\n"); 
//                    break;
//				}
//
//				if (f_open(&DstFile, DstPath, FA_CREATE_ALWAYS | FA_WRITE))    /* Create destination file */
//                {	
//                    wprintf(L"Failed to create destination file.\n"); 
//                    break;
//				}
//
//				do   /* Copy source file to destination file */
//                {	
//					ReadFile(SrcFile, Buff, sizeof(Buff), &br, 0);
//					if (br == 0) 
//                        break;
//
//					f_write(&DstFile, Buff, (UINT)br, &bw);
//				} while (br == bw);
//
//				CloseHandle(SrcFile);
//				f_close(&DstFile);
//
//				if (br && br != bw) 
//                {
//					wprintf(L"Failed to write file.\n"); 
//                    break;
//				}
//				Files++;
//			}
//			if (!FindNextFile(hdir, &Fd)) 
//            {
//				rv = 1; 
//                break;
//			}
//		}
//		FindClose(hdir);
//	}
//	SrcPath[slen] = 0;
//	DstPath[dlen] = 0;
//	return rv;
//}

int wmain()
{
	FRESULT ret;
	DIR     dr = {0};
	RamDiskSize = 65526U+16*1024;
	
	ret = f_mount(&FatFs, L"", 0);

	ret = f_mkfs(L"", 1, 512);

	ret = f_mkdir(L"projects");
	ret = f_mkdir(L"projects/one");

	ret = f_opendir(&dr, L"projects/one");

}


//int wmain (int argc, wchar_t* argv[])
//{
//	UINT csz;
//	HANDLE wh;
//	DWORD wb, szvol;
//	int ai = 1, truncation = 0;
//	const wchar_t *outfile;
//
//	/* Initialize parameters */
//	if (argc >= 2 && *argv[ai] == '-') 
//    {
//		if (/*strcmp*/wcscmp(argv[ai], L"-t") == 0)
//        {
//			truncation = 1;
//			ai++;
//			argc--;
//		} 
//        else 
//        {
//			argc = 0;
//		}
//	}
//
//	if (argc < 3) 
//    {
//		wprintf(L"usage: mkfatimg [-t] <source node> <output file> <volume size> [<cluster size>]\n"
//				L"    -t: Truncate unused area for read only volume.\n"
//				L"    <source node>: Source node\n"
//				L"    <output file>: FAT volume image file\n"
//				L"    <volume size>: Size of temporary volume size in unit of KiB\n"
//				L"    <cluster size>: Size of cluster in unit of byte (default:512)\n");
//			
//		return 1;
//	}
//	
//    wcscpy(SrcPath, argv[ai++]);
//	outfile         = argv[ai++];
//	RamDiskSize     = _wtoi(argv[ai++]) * 2;
//
//	csz = (argc >= 5) ? _wtoi(argv[ai++]) : 512;
//
//	/* Create an FAT volume */
//	f_mount(&FatFs, L"", 0);
//	if (f_mkfs(L"", 1, csz)) 
//    {
//        wprintf(L"Failed to create FAT volume. Adjust volume size or cluster size.\n");
//		return 2;
//	}
//
//	/* Copy directory tree onto the FAT volume */
//	if (!maketree()) 
//        return 3;
//
//	if (!Files) 
//    { 
//        wprintf(L"No file in the source directory."); 
//        return 3; 
//    }
//
//	szvol = LD_WORD(RamDisk + BPB_TotSec16);
//	if (!szvol) 
//        szvol = LD_DWORD(RamDisk + BPB_TotSec32);
//
//	if (truncation) 
//    {
//		DWORD ent, nent;
//		DWORD szf, szfp, edf, edfp;
//		DWORD szd, szdp, edd, eddp;
//
//		/* Truncate unused root directory entries */
//		if (FatFs.fs_type != FS_FAT32) 
//        {
//			printf("\nTruncating unused root directory area...");
//			for (nent = ent = 0; ent < FatFs.n_rootdir; ent++) 
//            {
//				if (RamDisk[FatFs.dirbase * 512 + ent * 32]) 
//                    nent = ent + 1;
//			}
//			szd = (nent + 15) / 16;
//			szdp = FatFs.n_rootdir / 16;
//			if (szd < szdp) 
//            {
//				edd = FatFs.dirbase + szd;
//				eddp = FatFs.database;
//				MoveMemory(RamDisk + (edd * 512), RamDisk + (eddp * 512), (szvol - eddp) * 512);
//				szvol -= szdp - szd;
//				FatFs.database -= szdp - szd;
//				ST_WORD(RamDisk + BPB_RootEntCnt, szd * 16);
//			}
//		}
//
//		/* Truncate unused data area and FAT */
//		printf("\nTruncating unused data area...");
//		for (nent = ent = 2; ent < FatFs.n_fatent; ent++) 
//        {
//			if (get_fat(&FatFs, ent)) 
//                nent = ent + 1;
//		}
//
//		switch (FatFs.fs_type) 
//        {
//		    case FS_FAT12:
//		    	szf = (nent * 3 / 2 + (nent & 1) + 511) / 512;
//		    	break;
//		    case FS_FAT16:
//		    	szf = (nent * 2 + 511) / 512;
//		    	if (nent - 2 < MIN_FAT16) nent = 0;
//		    	break;
//		    default:
//		    	szf = (nent * 4 + 511) / 512;
//		    	if (nent - 2 < MIN_FAT32) 
//                    nent = 0;
//		    	break;
//		}
//
//		if (!nent) 
//        {
//			printf("different FAT sub-type is needed. Adjust volume size or cluster size.\n");
//			return 3;
//		}
//
//		szfp = LD_WORD(RamDisk + BPB_FATSz16) * RamDisk[BPB_NumFATs];
//		if (!szfp) 
//            szfp = LD_DWORD(RamDisk + BPB_FATSz32) * RamDisk[BPB_NumFATs];
//
//		edf  = FatFs.fatbase + szf;
//		edfp = (FatFs.fs_type == FS_FAT32) ? FatFs.database : FatFs.dirbase;
//		MoveMemory(RamDisk + (edf * 512), RamDisk + (edfp * 512), (szvol - edfp) * 512);
//		szvol -= (szfp - szf) + FatFs.csize * (FatFs.n_fatent - nent);
//		if (FatFs.fs_type == FS_FAT32) 
//        {
//			ST_DWORD(RamDisk + BPB_FATSz32, szf);
//		} 
//        else 
//        {
//			ST_WORD(RamDisk + BPB_FATSz16, szf);
//		}
//
//		RamDisk[BPB_NumFATs] = 1;
//		if (szvol < 0x10000) 
//        {
//			ST_WORD(RamDisk + BPB_TotSec16, szvol);
//			ST_DWORD(RamDisk + BPB_TotSec32, 0);
//		} 
//        else 
//        {
//			ST_WORD(RamDisk + BPB_TotSec16, 0);
//			ST_DWORD(RamDisk + BPB_TotSec32, szvol);
//		}
//	}
//
//	/* Output the FAT volume to the file */
//	wprintf(L"\nWriting output file...");
//	wh = CreateFile(outfile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
//	if (wh == INVALID_HANDLE_VALUE) 
//    {
//		wprintf(L"Failed to create output file.\n");
//		return 4;
//	}
//
//	szvol *= 512;
//
//	WriteFile(wh, RamDisk, szvol, &wb, 0);
//	CloseHandle(wh);
//
//	if (szvol != wb) 
//    {
//		DeleteFile(argv[2]);
//		//printf("Failed to write output file.\n");
//        wprintf(L"Failed to write output file.\n");
//		return 4;
//	}
//
//	wprintf(L"\n%u files and %u directories in a %u bytes of FAT volume.\n", Files, Dirs, szvol);
//
//	return 0;
//}