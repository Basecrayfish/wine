/*
 * Unit tests for file functions in Wine
 *
 * Copyright (c) 2002 Jakob Eriksson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdlib.h>
#include <time.h>

#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"


LPCSTR filename = "testfile.xxx";
LPCSTR sillytext =
"en larvig liten text dx \033 gx hej 84 hej 4484 ! \001\033 bla bl\na.. bla bla."
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"1234 43 4kljf lf &%%%&&&&&& 34 4 34   3############# 33 3 3 3 # 3## 3"
"sdlkfjasdlkfj a dslkj adsklf  \n  \nasdklf askldfa sdlkf \nsadklf asdklf asdf ";


static void test__hread( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    long bytes_wanted;
    long i;

    SetFileAttributesA(filename,FILE_ATTRIBUTE_NORMAL); /* be sure to remove stale files */
    DeleteFileA( filename );
    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    filehandle = _lopen( filename, OF_READ );

    ok( HFILE_ERROR != filehandle, "couldn't open file \"%s\" again (err=%ld)", filename, GetLastError(  ) );

    bytes_read = _hread( filehandle, buffer, 2 * strlen( sillytext ) );

    ok( lstrlenA( sillytext ) == bytes_read, "file read size error" );

    for (bytes_wanted = 0; bytes_wanted < lstrlenA( sillytext ); bytes_wanted++)
    {
        ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains" );
        ok( _hread( filehandle, buffer, bytes_wanted ) == bytes_wanted, "erratic _hread return value" );
        for (i = 0; i < bytes_wanted; i++)
        {
            ok( buffer[i] == sillytext[i], "that's not what's written" );
        }
    }

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)", GetLastError(  ) );
}


static void test__hwrite( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    long bytes_written;
    long blocks;
    long i;
    char *contents;
    HLOCAL memory_object;
    char checksum[1];

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, "", 0 ), "_hwrite complains" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    filehandle = _lopen( filename, OF_READ );

    bytes_read = _hread( filehandle, buffer, 1);

    ok( 0 == bytes_read, "file read size error" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    filehandle = _lopen( filename, OF_READWRITE );

    bytes_written = 0;
    checksum[0] = '\0';
    srand( (unsigned)time( NULL ) );
    for (blocks = 0; blocks < 100; blocks++)
    {
        for (i = 0; i < sizeof( buffer ); i++)
        {
            buffer[i] = rand(  );
            checksum[0] = checksum[0] + buffer[i];
        }
        ok( HFILE_ERROR != _hwrite( filehandle, buffer, sizeof( buffer ) ), "_hwrite complains" );
        bytes_written = bytes_written + sizeof( buffer );
    }

    ok( HFILE_ERROR != _hwrite( filehandle, checksum, 1 ), "_hwrite complains" );
    bytes_written++;

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains" );

    memory_object = LocalAlloc( LPTR, bytes_written );

    ok( 0 != memory_object, "LocalAlloc fails. (Could be out of memory.)" );

    contents = LocalLock( memory_object );

    filehandle = _lopen( filename, OF_READ );

    contents = LocalLock( memory_object );

    ok( NULL != contents, "LocalLock whines" );

    ok( bytes_written == _hread( filehandle, contents, bytes_written), "read length differ from write length" );

    checksum[0] = '\0';
    i = 0;
    do
    {
        checksum[0] = checksum[0] + contents[i];
        i++;
    }
    while (i < bytes_written - 1);

    ok( checksum[0] == contents[i], "stored checksum differ from computed checksum" );

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)", GetLastError(  ) );
}


static void test__lclose( void )
{
    HFILE filehandle;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)", GetLastError(  ) );
}


static void test__lcreat( void )
{
    HFILE filehandle;
    char buffer[10000];
    WIN32_FIND_DATAA search_results;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains" );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains" );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  lstrlenA( sillytext ), "erratic _hread return value" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should be able to find file" );

    ok( DeleteFileA(filename) != 0, "DeleteFile failed (%ld)", GetLastError());

    filehandle = _lcreat( filename, 1 ); /* readonly */
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%ld)", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite shouldn't be able to write never the less" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should be able to find file" );

    ok( 0 == DeleteFileA( filename ), "shouldn't be able to delete a readonly file" );

    ok( SetFileAttributesA(filename, FILE_ATTRIBUTE_NORMAL ) != 0, "couldn't change attributes on file" );

    ok( DeleteFileA( filename ) != 0, "now it should be possible to delete the file!" );

    filehandle = _lcreat( filename, 2 );
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%ld)", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains" );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains" );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  lstrlenA( sillytext ), "erratic _hread return value" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should STILL be able to find file" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)", GetLastError(  ) );

    filehandle = _lcreat( filename, 4 ); /* SYSTEM file */
    ok( HFILE_ERROR != filehandle, "couldn't create file \"%s\" (err=%ld)", filename, GetLastError(  ) );

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains" );

    ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains" );

    ok( _hread( filehandle, buffer, strlen( sillytext ) ) ==  lstrlenA( sillytext ), "erratic _hread return value" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    ok( INVALID_HANDLE_VALUE != FindFirstFileA( filename, &search_results ), "should STILL be able to find file" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)", GetLastError(  ) );
}


void test__llseek( void )
{
    INT i;
    HFILE filehandle;
    char buffer[1];
    long bytes_read;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)",filename,GetLastError());
        return;
    }

    for (i = 0; i < 400; i++)
    {
        ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains" );
    }
    ok( HFILE_ERROR != _llseek( filehandle, 400 * strlen( sillytext ), FILE_CURRENT ), "should be able to seek" );
    ok( HFILE_ERROR != _llseek( filehandle, 27 + 35 * strlen( sillytext ), FILE_BEGIN ), "should be able to seek" );

    bytes_read = _hread( filehandle, buffer, 1);
    ok( 1 == bytes_read, "file read size error" );
    ok( buffer[0] == sillytext[27], "_llseek error, it got lost seeking" );
    ok( HFILE_ERROR != _llseek( filehandle, -400 * strlen( sillytext ), FILE_END ), "should be able to seek" );

    bytes_read = _hread( filehandle, buffer, 1);
    ok( 1 == bytes_read, "file read size error" );
    ok( buffer[0] == sillytext[0], "_llseek error, it got lost seeking" );
    ok( HFILE_ERROR != _llseek( filehandle, 1000000, FILE_END ), "should be able to seek past file; poor, poor Windows programmers" );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)", GetLastError(  ) );
}


static void test__llopen( void )
{
    HFILE filehandle;
    UINT bytes_read;
    char buffer[10000];

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains" );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    filehandle = _lopen( filename, OF_READ );
    ok( HFILE_ERROR == _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite shouldn't be able to write!" );
    bytes_read = _hread( filehandle, buffer, strlen( sillytext ) );
    ok( strlen( sillytext )  == bytes_read, "file read size error" );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    filehandle = _lopen( filename, OF_READWRITE );
    bytes_read = _hread( filehandle, buffer, 2 * strlen( sillytext ) );
    ok( strlen( sillytext )  == bytes_read, "file read size error" );
    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite should write just fine" );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    filehandle = _lopen( filename, OF_WRITE );
    ok( HFILE_ERROR == _hread( filehandle, buffer, 1 ), "you should only be able to write this file" );
    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite should write just fine" );
    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)", GetLastError(  ) );
    /* TODO - add tests for the SHARE modes  -  use two processes to pull this one off */
}


static void test__lread( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    UINT bytes_wanted;
    UINT i;

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _hwrite( filehandle, sillytext, strlen( sillytext ) ), "_hwrite complains" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    filehandle = _lopen( filename, OF_READ );

    ok( HFILE_ERROR != filehandle, "couldn't open file \"%s\" again (err=%ld)", filename, GetLastError());

    bytes_read = _lread( filehandle, buffer, 2 * strlen( sillytext ) );

    ok( lstrlenA( sillytext ) == bytes_read, "file read size error" );

    for (bytes_wanted = 0; bytes_wanted < strlen( sillytext ); bytes_wanted++)
    {
        ok( 0 == _llseek( filehandle, 0, FILE_BEGIN ), "_llseek complains" );
        ok( _lread( filehandle, buffer, bytes_wanted ) == bytes_wanted, "erratic _hread return value" );
        for (i = 0; i < bytes_wanted; i++)
        {
            ok( buffer[i] == sillytext[i], "that's not what's written" );
        }
    }

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)", GetLastError(  ) );
}


static void test__lwrite( void )
{
    HFILE filehandle;
    char buffer[10000];
    long bytes_read;
    long bytes_written;
    long blocks;
    long i;
    char *contents;
    HLOCAL memory_object;
    char checksum[1];

    filehandle = _lcreat( filename, 0 );
    if (filehandle == HFILE_ERROR)
    {
        ok(0,"couldn't create file \"%s\" (err=%ld)",filename,GetLastError());
        return;
    }

    ok( HFILE_ERROR != _lwrite( filehandle, "", 0 ), "_hwrite complains" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    filehandle = _lopen( filename, OF_READ );

    bytes_read = _hread( filehandle, buffer, 1);

    ok( 0 == bytes_read, "file read size error" );

    ok( HFILE_ERROR != _lclose(filehandle), "_lclose complains" );

    filehandle = _lopen( filename, OF_READWRITE );

    bytes_written = 0;
    checksum[0] = '\0';
    srand( (unsigned)time( NULL ) );
    for (blocks = 0; blocks < 100; blocks++)
    {
        for (i = 0; i < sizeof( buffer ); i++)
        {
            buffer[i] = rand(  );
            checksum[0] = checksum[0] + buffer[i];
        }
        ok( HFILE_ERROR != _lwrite( filehandle, buffer, sizeof( buffer ) ), "_hwrite complains" );
        bytes_written = bytes_written + sizeof( buffer );
    }

    ok( HFILE_ERROR != _lwrite( filehandle, checksum, 1 ), "_hwrite complains" );
    bytes_written++;

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains" );

    memory_object = LocalAlloc( LPTR, bytes_written );

    ok( 0 != memory_object, "LocalAlloc fails, could be out of memory" );

    contents = LocalLock( memory_object );

    filehandle = _lopen( filename, OF_READ );

    contents = LocalLock( memory_object );

    ok( NULL != contents, "LocalLock whines" );

    ok( bytes_written == _hread( filehandle, contents, bytes_written), "read length differ from write length" );

    checksum[0] = '\0';
    i = 0;
    do
    {
        checksum[0] += contents[i];
        i++;
    }
    while (i < bytes_written - 1);

    ok( checksum[0] == contents[i], "stored checksum differ from computed checksum" );

    ok( HFILE_ERROR != _lclose( filehandle ), "_lclose complains" );

    ok( DeleteFileA( filename ) != 0, "DeleteFile failed (%ld)", GetLastError(  ) );
}

void test_CopyFileA(void)
{
    char temp_path[MAX_PATH];
    char source[MAX_PATH], dest[MAX_PATH];
    static const char prefix[] = "pfx";
    DWORD ret;

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %ld", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH");

    ret = GetTempFileNameA(temp_path, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameA error %ld", GetLastError());

    ret = GetTempFileNameA(temp_path, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameA error %ld", GetLastError());

    ret = CopyFileA(source, dest, TRUE);
    ok(!ret && GetLastError() == ERROR_FILE_EXISTS,
       "CopyFileA: unexpected error %ld\n", GetLastError());

    ret = CopyFileA(source, dest, FALSE);
    ok(ret,  "CopyFileA: error %ld\n", GetLastError());

    ret = DeleteFileA(source);
    ok(ret, "DeleteFileA: error %ld\n", GetLastError());
    ret = DeleteFileA(dest);
    ok(ret, "DeleteFileA: error %ld\n", GetLastError());
}

void test_CopyFileW(void)
{
    WCHAR temp_path[MAX_PATH];
    WCHAR source[MAX_PATH], dest[MAX_PATH];
    static const WCHAR prefix[] = {'p','f','x',0};
    DWORD ret;

    ret = GetTempPathW(MAX_PATH, temp_path);
    if (ret==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(ret != 0, "GetTempPathW error %ld", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH");

    ret = GetTempFileNameW(temp_path, prefix, 0, source);
    ok(ret != 0, "GetTempFileNameW error %ld", GetLastError());

    ret = GetTempFileNameW(temp_path, prefix, 0, dest);
    ok(ret != 0, "GetTempFileNameW error %ld", GetLastError());

    ret = CopyFileW(source, dest, TRUE);
    ok(!ret && GetLastError() == ERROR_FILE_EXISTS,
       "CopyFileW: unexpected error %ld\n", GetLastError());

    ret = CopyFileW(source, dest, FALSE);
    ok(ret,  "CopyFileW: error %ld\n", GetLastError());

    ret = DeleteFileW(source);
    ok(ret, "DeleteFileW: error %ld\n", GetLastError());
    ret = DeleteFileW(dest);
    ok(ret, "DeleteFileW: error %ld\n", GetLastError());
}

void test_CreateFileA(void)
{
    HANDLE hFile;
    char temp_path[MAX_PATH];
    char filename[MAX_PATH];
    static const char prefix[] = "pfx";
    DWORD ret;

    ret = GetTempPathA(MAX_PATH, temp_path);
    ok(ret != 0, "GetTempPathA error %ld", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH");

    ret = GetTempFileNameA(temp_path, prefix, 0, filename);
    ok(ret != 0, "GetTempFileNameA error %ld", GetLastError());

    hFile = CreateFileA(filename, GENERIC_READ, 0, NULL,
                        CREATE_NEW, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS,
        "CREATE_NEW should fail if file exists and last error value should be ERROR_FILE_EXISTS");

    ret = DeleteFileA(filename);
    ok(ret, "DeleteFileA: error %ld\n", GetLastError());
}

void test_CreateFileW(void)
{
    HANDLE hFile;
    WCHAR temp_path[MAX_PATH];
    WCHAR filename[MAX_PATH];
    static const WCHAR prefix[] = {'p','f','x',0};
    DWORD ret;

    ret = GetTempPathW(MAX_PATH, temp_path);
    if (ret==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(ret != 0, "GetTempPathW error %ld", GetLastError());
    ok(ret < MAX_PATH, "temp path should fit into MAX_PATH");

    ret = GetTempFileNameW(temp_path, prefix, 0, filename);
    ok(ret != 0, "GetTempFileNameW error %ld", GetLastError());

    hFile = CreateFileW(filename, GENERIC_READ, 0, NULL,
                        CREATE_NEW, FILE_FLAG_RANDOM_ACCESS, 0);
    ok(hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS,
        "CREATE_NEW should fail if file exists and last error value should be ERROR_FILE_EXISTS");

    ret = DeleteFileW(filename);
    ok(ret, "DeleteFileW: error %ld\n", GetLastError());
}

static void test_DeleteFileA( void )
{
    BOOL ret;

    ret = DeleteFileA(NULL);
    ok(!ret && (GetLastError() == ERROR_INVALID_PARAMETER ||
                GetLastError() == ERROR_PATH_NOT_FOUND),
       "DeleteFileA(NULL) returned ret=%d error=%ld",ret,GetLastError());

    ret = DeleteFileA("");
    ok(!ret && (GetLastError() == ERROR_PATH_NOT_FOUND ||
                GetLastError() == ERROR_BAD_PATHNAME),
       "DeleteFileA(\"\") returned ret=%d error=%ld",ret,GetLastError());
}

static void test_DeleteFileW( void )
{
    BOOL ret;
    WCHAR emptyW[]={'\0'};

    ret = DeleteFileW(NULL);
    if (ret==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
       "DeleteFileW(NULL) returned ret=%d error=%ld",ret,GetLastError());

    ret = DeleteFileW(emptyW);
    ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
       "DeleteFileW(\"\") returned ret=%d error=%ld",ret,GetLastError());
}

#define PATTERN_OFFSET 0x10

void test_offset_in_overlapped_structure(void)
{
    HANDLE hFile;
    OVERLAPPED ov;
    DWORD done;
    BOOL rc;
    BYTE buf[256], pattern[] = "TeSt";
    UINT i;
    char temp_path[MAX_PATH], temp_fname[MAX_PATH];

    ok(GetTempPathA(MAX_PATH, temp_path) != 0, "GetTempPathA error %ld", GetLastError());
    ok(GetTempFileNameA(temp_path, "pfx", 0, temp_fname) != 0, "GetTempFileNameA error %ld", GetLastError());

    /*** Write File *****************************************************/

    hFile = CreateFileA(temp_fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA error %ld", GetLastError());

    for(i = 0; i < sizeof(buf); i++) buf[i] = i;
    ok(WriteFile(hFile, buf, sizeof(buf), &done, NULL), "WriteFile error %ld", GetLastError());
    ok(done == sizeof(buf), "expected number of bytes written %lu", done);

    memset(&ov, 0, sizeof(ov));
    ov.Offset = PATTERN_OFFSET;
    ov.OffsetHigh = 0;
    rc=WriteFile(hFile, pattern, sizeof(pattern), &done, &ov);
    /* Win 9x does not support the overlapped I/O on files */
    if (rc || GetLastError()!=ERROR_INVALID_PARAMETER) {
        ok(rc, "WriteFile error %ld", GetLastError());
        ok(done == sizeof(pattern), "expected number of bytes written %lu", done);
        trace("Current offset = %04lx\n", SetFilePointer(hFile, 0, NULL, FILE_CURRENT));
        ok(SetFilePointer(hFile, 0, NULL, FILE_CURRENT) == (PATTERN_OFFSET + sizeof(pattern)),
           "expected file offset %d", PATTERN_OFFSET + sizeof(pattern));

        ov.Offset = sizeof(buf) * 2;
        ov.OffsetHigh = 0;
        ok(WriteFile(hFile, pattern, sizeof(pattern), &done, &ov), "WriteFile error %ld", GetLastError());
        ok(done == sizeof(pattern), "expected number of bytes written %lu", done);
        /*trace("Current offset = %04lx\n", SetFilePointer(hFile, 0, NULL, FILE_CURRENT));*/
        ok(SetFilePointer(hFile, 0, NULL, FILE_CURRENT) == (sizeof(buf) * 2 + sizeof(pattern)),
           "expected file offset %d", sizeof(buf) * 2 + sizeof(pattern));
    }

    CloseHandle(hFile);

    /*** Read File *****************************************************/

    hFile = CreateFileA(temp_fname, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFileA error %ld", GetLastError());

    memset(buf, 0, sizeof(buf));
    memset(&ov, 0, sizeof(ov));
    ov.Offset = PATTERN_OFFSET;
    ov.OffsetHigh = 0;
    rc=ReadFile(hFile, buf, sizeof(pattern), &done, &ov);
    /* Win 9x does not support the overlapped I/O on files */
    if (rc || GetLastError()!=ERROR_INVALID_PARAMETER) {
        ok(rc, "ReadFile error %ld", GetLastError());
        ok(done == sizeof(pattern), "expected number of bytes read %lu", done);
        trace("Current offset = %04lx\n", SetFilePointer(hFile, 0, NULL, FILE_CURRENT));
        ok(SetFilePointer(hFile, 0, NULL, FILE_CURRENT) == (PATTERN_OFFSET + sizeof(pattern)),
           "expected file offset %d", PATTERN_OFFSET + sizeof(pattern));
        ok(!memcmp(buf, pattern, sizeof(pattern)), "pattern match failed");
    }

    CloseHandle(hFile);

    ok(DeleteFileA(temp_fname), "DeleteFileA error %ld\n", GetLastError());
}

START_TEST(file)
{
    test__hread(  );
    test__hwrite(  );
    test__lclose(  );
    test__lcreat(  );
    test__llseek(  );
    test__llopen(  );
    test__lread(  );
    test__lwrite(  );
    test_CopyFileA();
    test_CopyFileW();
    test_CreateFileA();
    test_CreateFileW();
    test_DeleteFileA();
    test_DeleteFileW();
    test_offset_in_overlapped_structure();
}
