#include "fatfs_ext/inc/ff_api.h"
#include "fatfs_ext/inc/ff_driver.h"
#include "basic_types.h"

#include "platform_stdlib.h"

enum {
	MOUNT_LATER = 0,
	MOUNT_NOW,
};

FATFS 	m_fs;
FIL		m_file[2];
TCHAR	root[4];	/* root diretor */
TCHAR	ab_path[64]; /* absoluty path */

DWORD Size_cnt;				/* Work register for fs command */
WORD File_cnt, Dir_cnt;

static const BYTE ft[] = {0, 12, 16, 32};

BYTE Buff[4096]; /* this buf used to cache data */

static
void error (FRESULT res)
{
	const char *str =
		"DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" 
		"NO_PATH\0" "INVALID_NAME\0" "DENIED\0" "EXIST\0" 
		"INVALID_OBJECT\0" "WRITE_PROTECTED\0" "INVALID_DRIVE\0"
		"NOT_ENABLED\0" "NO_FILESYSTEM\0" "MKFS_ABORTED\0" 
		"TIMEOUT\0" "LOCKED\0" "NOT_ENOUGH_CORE\0" 
		"TOO_MANY_OPEN_FILES\0" "INVALID_PARAMETER\0";
	FRESULT i;

	for (i = 1; i != res && *str; i++) {
		while (*str++) ;
	}
	DBG_8195A("\tFR_%s\n", str);
}

static FRESULT scan_files (char* path)
{
	FRESULT res;
	FILINFO fno;
	DIR dir;
	int i;
	char *fn;
#if _USE_LFN
	static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
	fno.lfname = lfn;
	fno.lfsize = sizeof(lfn);
#endif
	res = f_opendir(&dir, path);
	if (res == FR_OK) {
		i = strlen(path);
		for (;;) {
			res = f_readdir(&dir, &fno);
			if (res != FR_OK || fno.fname[0] == 0) break;
		 if (fno.fname[0] == '.') continue;
#if _USE_LFN
			fn = *fno.lfname ? fno.lfname : fno.fname;
#else
			fn = fno.fname;
#endif
			if (fno.fattrib & AM_DIR) {  // if this path is a folder
				Dir_cnt ++;
				sprintf(&path[i],"/%s",fn);
				res = scan_files(path);
				if (res != FR_OK) break;
				path[i] = 0;
			} else {
				File_cnt ++;
				Size_cnt += fno.fsize;
			}
		}
	}
	return res;
}


uint cmd_fatfs_init_volume(BYTE* d_name ,BYTE status){
	if(status){
		if(FATFS_RegisterDriver(d_name, root))
			return FAIL;
		if(f_mount(&m_fs, root, MOUNT_NOW)!= FR_OK){
			FATFS_UnRegisterDriver(root);
			return FAIL;
		}
		_strcpy(ab_path, root); 
		return SUCCESS;
	}
	else
		if(f_mount(NULL, root, MOUNT_NOW) == FR_OK){
			if(FATFS_UnRegisterDriver(root))
				return FAIL;
			return SUCCESS;
		}
}

uint cmd_fatfs_open_file(char *filename, BYTE mode){
	FRESULT res;
	TCHAR path[64];
	
	_memset(path, 0, 64);
	_strcpy(path, ab_path);

	sprintf(&path[strlen(path)],"/%s",filename);
	
	res = f_open(&m_file[0], path, mode);
	if(res){error(res);return FAIL;}
	return SUCCESS;
}

uint cmd_fatfs_close_file(){
	FRESULT res;
	res = f_close(&m_file[0]);
	if(res){error(res);return FAIL;}
	return SUCCESS;
}

uint cmd_fatfs_copy_file(TCHAR *src_name, TCHAR *dst_name){
	FRESULT res;
	UINT  br,bw;

	TCHAR path1[64];
	TCHAR path2[64];
	
	_memset(path1, 0, 64);
	_memset(path2, 0, 64);
	_strcpy(path1, ab_path);
	_strcpy(path2, ab_path);

	sprintf(&path1[strlen(path1)],"/%s",src_name);
	sprintf(&path2[strlen(path2)],"/%s",dst_name);
	
	res = f_open(&m_file[0], path1, FA_OPEN_EXISTING | FA_READ);
	if (res) {error(res);return FAIL;}
	res = f_open(&m_file[1], path2, FA_CREATE_ALWAYS | FA_WRITE);
	if (res) {
		error(res);
		f_close(&m_file[0]);
		return FAIL;
	}
	for (;;) {
		res = f_read(&m_file[0], Buff, 4096, &br);
		if (res || br == 0) break;   /* error or eof */
		res = f_write(&m_file[1], Buff, br, &bw);
		if (res || bw < br) break;   /* error or disk full */
	}
	f_close(&m_file[0]);
	f_close(&m_file[1]);
	return SUCCESS;
}
uint cmd_fatfs_chmod(TCHAR *filename, BYTE attri){
	FRESULT res;
	TCHAR path[64];
	
	_memset(path, 0, 64);
	_strcpy(path, ab_path);

	sprintf(&path[strlen(path)],"/%s",filename);
		
	res = f_chmod(path, attri, AM_RDO|AM_ARC|AM_SYS|AM_HID);
	if(res){error(res);return FAIL;}
	return SUCCESS;
}

uint cmd_fatfs_rename(TCHAR *oldname, TCHAR *newname){
	FRESULT res;
	TCHAR path_old[64],path_new[64];
	
	_memset(path_old, 0, 64);
	_memset(path_new, 0, 64);
	_strcpy(path_old, ab_path);
	_strcpy(path_new, ab_path);

	sprintf(&path_old[strlen(path_old)],"/%s",oldname);
	sprintf(&path_new[strlen(path_new)],"/%s",newname);

	res = f_rename(path_old, path_new);
	if(res){error(res);return FAIL;}
	return SUCCESS;
}

uint cmd_disk_getcapacity(void)
{
	FRESULT rc;

	FATFS *fs;
	DWORD fre_clust;

	rc = f_getfree(root, &fre_clust, &fs);
    if ( rc == FR_OK ){
    	DWORD    TotalSpace = (u16)(((fs->n_fatent - 2) * fs->csize ) / 2 /1024);
    	DWORD    AvailableSize = ((fre_clust * fs->csize) / 2 /1024);
    	DWORD    UsedSize = TotalSpace - AvailableSize;
#ifdef DUMP_FS_INFO
    	if(TotalSpace/1024 > 1)
                /* Print free space in unit of GB (assuming 512 bytes/sector) */
    		DiagPrintf("\n\t%d.%d GB total drive space."
                			"\n\t%d.%d GB available."
                			"\n\t%d.%d GB  used.\r\n",
                TotalSpace/1024,TotalSpace%1000, AvailableSize/1024,AvailableSize%1000, UsedSize/1024,UsedSize%1000);
    	else
                /* Print free space in unit of MB (assuming 512 bytes/sector) */
    		DiagPrintf("\n\t%d MB total drive space."
                			"\n\t%d MB available."
                			"\n\t%d MB  used.\r\n",
                TotalSpace, AvailableSize, UsedSize);
#endif
    }
    else{
        error(rc);
        DiagPrintf("Get Free Capacity Failed (%d)\r\n", rc);
		return FAIL;
	}
	return SUCCESS;
}


uint cmd_fatfs_make_director(TCHAR *dirname){
	FRESULT res;
	TCHAR path[64];
	int i = strlen(ab_path);
        
    _memset(path, 0, 64);
	_strcpy(path, ab_path);
        
	sprintf(&path[i],"/%s", dirname);

	res = f_mkdir(path);
	if (res) { error(res); return FAIL; }
	return SUCCESS;
}

uint cmd_fatfs_open_director(TCHAR *dirname, BYTE cmd){
	int i = strlen(ab_path);
	printf("path: %s\n", ab_path);
	switch(cmd){
		case MOVE_TO_ROOT:
			strcpy(ab_path, root);
			break;
		case MOVE_TO:
			strcpy(ab_path, root);
			sprintf(&ab_path[i], "%s", dirname);
			break;
		case MOVE_BACK:
			while (ab_path[--i] != '/')
				ab_path[i] = 0;
			ab_path[i] = 0;
			break;
		case MOVE_NEXT:
			sprintf(&ab_path[i], "%s", dirname);
			break;
		default:
			return FAIL;
	}
	printf("path: %s\n", ab_path);
	return SUCCESS;
}

/* director listing */
uint cmd_fatfs_list_director(void){
	FRESULT res;
  	FILINFO fno;
  	DIR dir;
    TCHAR path[64];
        
    _memset(path, 0, 64);
	_strcpy(path, ab_path);
        
	res = f_opendir(&dir, path);
	if (res) { error(res); return FAIL; }
	Dir_cnt = File_cnt = Size_cnt = 0;
	for(;;) {
		res = f_readdir(&dir, &fno);
		if ((res != FR_OK) || !fno.fname[0]) break;
		if (fno.fattrib & AM_DIR) {
			Dir_cnt ++;
		} else {
			File_cnt++; Size_cnt += fno.fsize;
		}
		DiagPrintf("\n%s\t\t%s%s%s%s%s %d/%02d/%02d %02d:%02d\t%d bytes\t\t%s\n",
				fno.fname,	
				(fno.fattrib & AM_DIR) ? "D" : "-",
				(fno.fattrib & AM_RDO) ? "R" : "-",
				(fno.fattrib & AM_HID) ? "H" : "-",
				(fno.fattrib & AM_SYS) ? "S" : "-",
				(fno.fattrib & AM_ARC) ? "A" : "-",
				(fno.fdate >> 9) + 1980, (fno.fdate >> 5) & 15, fno.fdate & 31,
				(fno.ftime >> 11), (fno.ftime >> 5) & 63,
				fno.fsize, 
#if _USE_LFN
				Lfname);
#else
				" ");
#endif
	}
	DiagPrintf("\n\rDirector Overall:"
			"\n\r\t%d Dir(s)" 
			"\n\r\t%d File(s)"
			"\n\r\t%d bytes total\n", Dir_cnt,File_cnt, Size_cnt );
	return SUCCESS;
}


/* show logical drive status */
uint cmd_fatfs_show_status(void){
	FRESULT res;
	DWORD free_cluster;
	FATFS *fs;
	TCHAR path[64] = {0};
	
	_memset(path, 0, 64);
	_strcpy(path, root);

	res = f_getfree(path, &free_cluster, &fs);
	if (res) { error(res); return FAIL;}
	DiagPrintf("\n\r\tLogical drive Overall:"
			"\n\r\tFAT type = FAT%d\n	\
			\r\tBytes/Cluster = %d\n	\
			\r\tNumber of FATs = %d\n	\
		 	\r\tRoot DIR entries = %d\n	\
		 	\r\tSectors/FAT = %d\n	\
		 	\r\tNumber of clusters = %d\n	\
			\r\tVolume start (lba) = %d\n	\
			\r\tFAT start (lba) = %d\n	\
			\r\tDIR start (lba,clustor) = %d\n	\
			\r\tData start (lba) = %d\n\n",
			ft[fs->fs_type & 3], (DWORD)fs->csize * 512, fs->n_fats,
			fs->n_rootdir, fs->fsize, (DWORD)fs->n_fatent - 2,
			fs->volbase, fs->fatbase, fs->dirbase, fs->database);
	Size_cnt = File_cnt = Dir_cnt = 0;
	res = scan_files(path);
	if (res) { error(res); return FAIL;}
	DiagPrintf("\n\r\tVolume Overall:\n"
			"\r\t%d Dir(s),\t%d File(s),\t%d Byte(s).\n"
			"\r\t%d KB total disk space.\n\r\t%d KB available.\n",
			Dir_cnt, File_cnt, Size_cnt,
			(fs->n_fatent - 2) * (fs->csize / 2), free_cluster * (fs->csize / 2)
	);
	return SUCCESS;
}

uint cmd_fatfs_truncate_file(void){
	FRESULT res;

	res = f_truncate(&m_file[0]);
	if (res){ error(res); return FAIL;}
	res = f_sync(&m_file[0]); // sync changes
	if (res){ error(res); return FAIL;}
	return SUCCESS;
}

/**
 * Seek the file Read/Write pointer
 *
 * Call this function to set or set file pointer, if running seccussfully, the current file pointer 
 * will be return
 */
uint cmd_fatfs_seek_pointer(DWORD offset, BYTE mode){
	FRESULT res;
	if(&m_file[0] != NULL){
		if(mode){// seek pointer if the offset not equal to 0
			res = f_lseek(&m_file[0], offset);
			if (res){ error(res); return FAIL;}
		}
		return m_file[0].fptr;
	}
	return FAIL;
}
u32 cmd_fatfs_read_file(char * read_buf, unsigned int length){
	UINT  br;
	FRESULT res;

	if(&m_file[0] != NULL){
		res = f_read(&m_file[0], read_buf, length, &br);
		if(res){error(res);return FAIL;}
		return br; // return the actual length read
	}
	else
		return FAIL;
}

u32 cmd_fatfs_write_file(char * write_buf, unsigned int length){
    UINT	bw;
    FRESULT res;
    if(&m_file[0] != NULL){
		do{
			res = f_write(&m_file[0], write_buf, length, &bw);
			if(res){error(res);}
		} while (bw < length);
    }
	else
 		return FAIL;
}

u32 cmd_fatfs_sync(){
    if(&m_file[0] != NULL){
		if(f_sync(&m_file[0]) == FR_OK){
			return SUCCESS;
		}else{
			return FAIL;
		}
    }
	else
 		return FAIL;
}

FRESULT remove_an_obj (TCHAR *OBJ){
	return f_unlink(OBJ);
}

FRESULT remove_all_obj (TCHAR* path){
	FRESULT res;
	FILINFO fno;
	DIR dir;
	int i;
	char *fn;
#if _USE_LFN
    static char lfn[_MAX_LFN * (_DF1S ? 2 : 1) + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif
    res = f_opendir(&dir, path);
    if (res == FR_OK) {
    	i = strlen(path);
        for (;;) {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0) break;
         if (fno.fname[0] == '.') continue;
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
			sprintf(&path[i],"/%s",fn);
			if(fno.fattrib & AM_DIR){ // if this path is folder
				res = remove_all_obj(path); // remove all sub item
				if(res == FR_OK)
					res = remove_an_obj(path); // remove the root item
			}
			else
				res = remove_an_obj(path); // remove the file directly
            path[i] = 0;
            if(res != FR_OK) break;
        }
    }
    return res;
}

FRESULT remove_obj (TCHAR *path)
{
	FRESULT res;
	TCHAR i;
	TCHAR obj_type = 1;  // 0: file 	1: folder
	for(i = 0;i < strlen(path);i ++){
		if (path[i] == '.'){
			obj_type = 0;
			break;
		}
	}
	i = strlen(path);
	if(obj_type){
		res = remove_all_obj(path); //remove all the files contained
		path[i] = 0;   //reset path
		if(res == FR_OK)
			return remove_an_obj(path); // remove the empty folder
		return res;
	}
	else
		return remove_an_obj(path);
}

uint cmd_fatfs_delete_obj(TCHAR *objname){
	TCHAR path[64]={0};
	FRESULT res;   /* Result code */
	
	_memset(path, 0, 64);
	_strcpy(path, ab_path);

	sprintf(&path[strlen(path)],"/%s",objname);
	 	
	res = remove_obj(path);
	if(res){error(res);return FAIL;}
	return SUCCESS;
}

uint cmd_disk_make_file(char *filename){
	FRESULT res;   /* Result code */
	TCHAR path[64]={0};
	int i = strlen(ab_path);
        
    _memset(path, 0, 64);
	_strcpy(path, ab_path);
        
	sprintf(&path[i],"/%s", filename);

	res = f_open(&m_file[0], path, FA_CREATE_NEW);
	if(res){error(res);return FAIL;}
	res = f_close(&m_file[0]); // close the file once create successfully
	if(res){error(res);return FAIL;}
}
