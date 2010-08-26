/*
 * Unit test suite for time functions.
 *
 * Copyright 2004 Uwe Bonnes
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wine/test.h"
#include "winbase.h"
#include "winnls.h"
#include "time.h"

#include <stdlib.h> /*setenv*/
#include <stdio.h> /*printf*/
#include <errno.h>

#define SECSPERDAY         86400
#define SECSPERHOUR        3600
#define SECSPERMIN         60
#define MINSPERHOUR        60
#define HOURSPERDAY        24

static __time32_t (__cdecl *p_mkgmtime32)(struct tm*);
static struct tm* (__cdecl *p_gmtime32)(__time32_t*);
static errno_t    (__cdecl *p_strtime_s)(char*,size_t);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    p_gmtime32 = (void*)GetProcAddress(hmod, "_gmtime32");
    p_mkgmtime32 = (void*)GetProcAddress(hmod, "_mkgmtime32");
    p_strtime_s = (void*)GetProcAddress(hmod, "_strtime_s");
}

static int get_test_year(time_t *start)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);

    /* compute start of year in seconds */
    *start = SECSPERDAY * ((tm->tm_year - 70) * 365 +
                           (tm->tm_year - 69) / 4 -
                           (tm->tm_year - 1) / 100 +
                           (tm->tm_year + 299) / 400);
    return tm->tm_year;
}

static void test_ctime(void)
{
    time_t badtime = -1;
    char* ret;
    ret = ctime(&badtime);
    ok(ret == NULL, "expected ctime to return NULL, got %s\n", ret);
}
static void test_gmtime(void)
{
    __time32_t valid, gmt;
    struct tm* gmt_tm;

    if(!p_gmtime32) {
        win_skip("Skipping _gmtime32 tests\n");
        return;
    }

    gmt = valid = 0;
    gmt_tm = p_gmtime32(&gmt);
    if(!gmt_tm) {
        ok(0, "_gmtime32() failed\n");
        return;
    }

    ok(((gmt_tm->tm_year == 70) && (gmt_tm->tm_mon  == 0) && (gmt_tm->tm_yday  == 0) &&
                (gmt_tm->tm_mday ==  1) && (gmt_tm->tm_wday == 4) && (gmt_tm->tm_hour  == 0) &&
                (gmt_tm->tm_min  ==  0) && (gmt_tm->tm_sec  == 0) && (gmt_tm->tm_isdst == 0)),
            "Wrong date:Year %4d mon %2d yday %3d mday %2d wday %1d hour%2d min %2d sec %2d dst %2d\n",
            gmt_tm->tm_year, gmt_tm->tm_mon, gmt_tm->tm_yday, gmt_tm->tm_mday, gmt_tm->tm_wday,
            gmt_tm->tm_hour, gmt_tm->tm_min, gmt_tm->tm_sec, gmt_tm->tm_isdst);

    if(!p_mkgmtime32) {
        win_skip("Skipping _mkgmtime32 tests\n");
        return;
    }

    gmt_tm->tm_wday = gmt_tm->tm_yday = 0;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %u\n", gmt);
    ok(gmt_tm->tm_wday == 4, "gmt_tm->tm_wday = %d\n", gmt_tm->tm_wday);
    ok(gmt_tm->tm_yday == 0, "gmt_tm->tm_yday = %d\n", gmt_tm->tm_yday);

    gmt_tm->tm_wday = gmt_tm->tm_yday = 0;
    gmt_tm->tm_isdst = -1;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %u\n", gmt);
    ok(gmt_tm->tm_wday == 4, "gmt_tm->tm_wday = %d\n", gmt_tm->tm_wday);
    ok(gmt_tm->tm_yday == 0, "gmt_tm->tm_yday = %d\n", gmt_tm->tm_yday);

    gmt_tm->tm_wday = gmt_tm->tm_yday = 0;
    gmt_tm->tm_isdst = 1;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %u\n", gmt);
    ok(gmt_tm->tm_wday == 4, "gmt_tm->tm_wday = %d\n", gmt_tm->tm_wday);
    ok(gmt_tm->tm_yday == 0, "gmt_tm->tm_yday = %d\n", gmt_tm->tm_yday);

    gmt = valid = 173921;
    gmt_tm = p_gmtime32(&gmt);
    if(!gmt_tm) {
        ok(0, "_gmtime32() failed\n");
        return;
    }

    gmt_tm->tm_isdst = -1;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %u\n", gmt);
    ok(gmt_tm->tm_wday == 6, "gmt_tm->tm_wday = %d\n", gmt_tm->tm_wday);
    ok(gmt_tm->tm_yday == 2, "gmt_tm->tm_yday = %d\n", gmt_tm->tm_yday);

    gmt_tm->tm_isdst = 1;
    gmt = p_mkgmtime32(gmt_tm);
    ok(gmt == valid, "gmt = %u\n", gmt);
}

static void test_mktime(void)
{
    TIME_ZONE_INFORMATION tzinfo;
    DWORD res =  GetTimeZoneInformation(&tzinfo);
    struct tm my_tm, sav_tm;
    time_t nulltime, local_time;
    char TZ_env[256];
    char buffer[64];
    int year;
    time_t ref, secs;

    year = get_test_year( &ref );
    ref += SECSPERDAY;

    ok (res != TIME_ZONE_ID_INVALID, "GetTimeZoneInformation failed\n");
    WideCharToMultiByte( CP_ACP, 0, tzinfo.StandardName, -1, buffer, sizeof(buffer), NULL, NULL );
    trace( "bias %d std %d dst %d zone %s\n",
           tzinfo.Bias, tzinfo.StandardBias, tzinfo.DaylightBias, buffer );
    /* Bias may be positive or negative, to use offset of one day */
    my_tm = *localtime(&ref);  /* retrieve current dst flag */
    secs = SECSPERDAY - tzinfo.Bias * SECSPERMIN;
    secs -= (my_tm.tm_isdst ? tzinfo.DaylightBias : tzinfo.StandardBias) * SECSPERMIN;
    my_tm.tm_mday = 1 + secs/SECSPERDAY;
    secs = secs % SECSPERDAY;
    my_tm.tm_hour = secs / SECSPERHOUR;
    secs = secs % SECSPERHOUR;
    my_tm.tm_min = secs / SECSPERMIN;
    secs = secs % SECSPERMIN;
    my_tm.tm_sec = secs;

    my_tm.tm_year = year;
    my_tm.tm_mon  =  0;

    sav_tm = my_tm;

    local_time = mktime(&my_tm);
    ok(local_time == ref, "mktime returned %u, expected %u\n",
       (DWORD)local_time, (DWORD)ref);
    /* now test some unnormalized struct tm's */
    my_tm = sav_tm;
    my_tm.tm_sec += 60;
    my_tm.tm_min -= 1;
    local_time = mktime(&my_tm);
    ok(local_time == ref, "Unnormalized mktime returned %u, expected %u\n",
        (DWORD)local_time, (DWORD)ref);
    ok( my_tm.tm_year == sav_tm.tm_year && my_tm.tm_mon == sav_tm.tm_mon &&
        my_tm.tm_mday == sav_tm.tm_mday && my_tm.tm_hour == sav_tm.tm_hour &&
        my_tm.tm_sec == sav_tm.tm_sec,
            "mktime returned %2d-%02d-%02d %02d:%02d expected %2d-%02d-%02d %02d:%02d\n",
            my_tm.tm_year,my_tm.tm_mon,my_tm.tm_mday,
            my_tm.tm_hour,my_tm.tm_sec,
            sav_tm.tm_year,sav_tm.tm_mon,sav_tm.tm_mday,
            sav_tm.tm_hour,sav_tm.tm_sec);
    my_tm = sav_tm;
    my_tm.tm_min -= 60;
    my_tm.tm_hour += 1;
    local_time = mktime(&my_tm);
    ok(local_time == ref, "Unnormalized mktime returned %u, expected %u\n",
       (DWORD)local_time, (DWORD)ref);
    ok( my_tm.tm_year == sav_tm.tm_year && my_tm.tm_mon == sav_tm.tm_mon &&
        my_tm.tm_mday == sav_tm.tm_mday && my_tm.tm_hour == sav_tm.tm_hour &&
        my_tm.tm_sec == sav_tm.tm_sec,
            "mktime returned %2d-%02d-%02d %02d:%02d expected %2d-%02d-%02d %02d:%02d\n",
            my_tm.tm_year,my_tm.tm_mon,my_tm.tm_mday,
            my_tm.tm_hour,my_tm.tm_sec,
            sav_tm.tm_year,sav_tm.tm_mon,sav_tm.tm_mday,
            sav_tm.tm_hour,sav_tm.tm_sec);
    my_tm = sav_tm;
    my_tm.tm_mon -= 12;
    my_tm.tm_year += 1;
    local_time = mktime(&my_tm);
    ok(local_time == ref, "Unnormalized mktime returned %u, expected %u\n",
       (DWORD)local_time, (DWORD)ref);
    ok( my_tm.tm_year == sav_tm.tm_year && my_tm.tm_mon == sav_tm.tm_mon &&
        my_tm.tm_mday == sav_tm.tm_mday && my_tm.tm_hour == sav_tm.tm_hour &&
        my_tm.tm_sec == sav_tm.tm_sec,
            "mktime returned %2d-%02d-%02d %02d:%02d expected %2d-%02d-%02d %02d:%02d\n",
            my_tm.tm_year,my_tm.tm_mon,my_tm.tm_mday,
            my_tm.tm_hour,my_tm.tm_sec,
            sav_tm.tm_year,sav_tm.tm_mon,sav_tm.tm_mday,
            sav_tm.tm_hour,sav_tm.tm_sec);
    my_tm = sav_tm;
    my_tm.tm_mon += 12;
    my_tm.tm_year -= 1;
    local_time = mktime(&my_tm);
    ok(local_time == ref, "Unnormalized mktime returned %u, expected %u\n",
       (DWORD)local_time, (DWORD)ref);
    ok( my_tm.tm_year == sav_tm.tm_year && my_tm.tm_mon == sav_tm.tm_mon &&
        my_tm.tm_mday == sav_tm.tm_mday && my_tm.tm_hour == sav_tm.tm_hour &&
        my_tm.tm_sec == sav_tm.tm_sec,
            "mktime returned %2d-%02d-%02d %02d:%02d expected %2d-%02d-%02d %02d:%02d\n",
            my_tm.tm_year,my_tm.tm_mon,my_tm.tm_mday,
            my_tm.tm_hour,my_tm.tm_sec,
            sav_tm.tm_year,sav_tm.tm_mon,sav_tm.tm_mday,
            sav_tm.tm_hour,sav_tm.tm_sec);
    /* now a bad time example */
    my_tm = sav_tm;
    my_tm.tm_year = 69;
    local_time = mktime(&my_tm);
    ok((local_time == -1), "(bad time) mktime returned %d, expected -1\n", (int)local_time);

    my_tm = sav_tm;
    /* TEST that we are independent from the TZ variable */
    /*Argh, msvcrt doesn't have setenv() */
    _snprintf(TZ_env,255,"TZ=%s",(getenv("TZ")?getenv("TZ"):""));
    putenv("TZ=GMT");
    nulltime = mktime(&my_tm);
    ok(nulltime == ref,"mktime returned 0x%08x\n",(DWORD)nulltime);
    putenv(TZ_env);
}

static void test_localtime(void)
{
    TIME_ZONE_INFORMATION tzinfo;
    DWORD res =  GetTimeZoneInformation(&tzinfo);
    time_t gmt, ref;

    char TZ_env[256];
    struct tm* lt;
    int year = get_test_year( &ref );
    int is_leap = !(year % 4) && ((year % 100) || !((year + 300) % 400));

    gmt = ref + SECSPERDAY + tzinfo.Bias * SECSPERMIN;
    ok (res != TIME_ZONE_ID_INVALID, "GetTimeZoneInformation failed\n");
    lt = localtime(&gmt);
    gmt += (lt->tm_isdst ? tzinfo.DaylightBias : tzinfo.StandardBias) * SECSPERMIN;
    lt = localtime(&gmt);
    ok(((lt->tm_year == year) && (lt->tm_mon  == 0) && (lt->tm_yday  == 1) &&
	(lt->tm_mday ==  2) && (lt->tm_hour  == 0) &&
	(lt->tm_min  ==  0) && (lt->tm_sec  == 0)),
       "Wrong date:Year %d mon %d yday %d mday %d wday %d hour %d min %d sec %d dst %d\n",
       lt->tm_year, lt->tm_mon, lt->tm_yday, lt->tm_mday, lt->tm_wday, lt->tm_hour, 
       lt->tm_min, lt->tm_sec, lt->tm_isdst); 

    _snprintf(TZ_env,255,"TZ=%s",(getenv("TZ")?getenv("TZ"):""));
    putenv("TZ=GMT");
    lt = localtime(&gmt);
    ok(((lt->tm_year == year) && (lt->tm_mon  == 0) && (lt->tm_yday  == 1) &&
	(lt->tm_mday ==  2) && (lt->tm_hour  == 0) &&
	(lt->tm_min  ==  0) && (lt->tm_sec  == 0)),
       "Wrong date:Year %d mon %d yday %d mday %d wday %d hour %d min %d sec %d dst %d\n",
       lt->tm_year, lt->tm_mon, lt->tm_yday, lt->tm_mday, lt->tm_wday, lt->tm_hour, 
       lt->tm_min, lt->tm_sec, lt->tm_isdst); 
    putenv(TZ_env);

    /* June 22 */
    gmt = ref + 202 * SECSPERDAY + tzinfo.Bias * SECSPERMIN;
    lt = localtime(&gmt);
    gmt += (lt->tm_isdst ? tzinfo.DaylightBias : tzinfo.StandardBias) * SECSPERMIN;
    lt = localtime(&gmt);
    ok(((lt->tm_year == year) && (lt->tm_mon  == 6) && (lt->tm_yday  == 202) &&
	(lt->tm_mday == 22 - is_leap) && (lt->tm_hour  == 0) &&
	(lt->tm_min  ==  0) && (lt->tm_sec  == 0)),
       "Wrong date:Year %d mon %d yday %d mday %d wday %d hour %d min %d sec %d dst %d\n",
       lt->tm_year, lt->tm_mon, lt->tm_yday, lt->tm_mday, lt->tm_wday, lt->tm_hour, 
       lt->tm_min, lt->tm_sec, lt->tm_isdst); 
}

static void test_strdate(void)
{
    char date[16], * result;
    int month, day, year, count, len;

    result = _strdate(date);
    ok(result == date, "Wrong return value\n");
    len = strlen(date);
    ok(len == 8, "Wrong length: returned %d, should be 8\n", len);
    count = sscanf(date, "%02d/%02d/%02d", &month, &day, &year);
    ok(count == 3, "Wrong format: count = %d, should be 3\n", count);
}

static void test_strtime(void)
{
    char time[16], * result;
    int hour, minute, second, count, len;
    errno_t err;

    result = _strtime(time);
    ok(result == time, "Wrong return value\n");
    len = strlen(time);
    ok(len == 8, "Wrong length: returned %d, should be 8\n", len);
    count = sscanf(time, "%02d:%02d:%02d", &hour, &minute, &second);
    ok(count == 3, "Wrong format: count = %d, should be 3\n", count);

    if(!p_strtime_s) {
        win_skip("Skipping _strtime_s tests\n");
        return;
    }

    errno = 0;
    err = p_strtime_s(NULL, 0);
    ok(err == EINVAL, "err = %d\n", err);
    ok(errno == EINVAL, "errno = %d\n", errno);

    err = p_strtime_s(NULL, 1);
    ok(err == EINVAL, "err = %d\n", err);
    ok(errno == EINVAL, "errno = %d\n", errno);

    time[0] = 'x';
    err = p_strtime_s(time, 8);
    ok(err == ERANGE, "err = %d\n", err);
    ok(errno == ERANGE, "errno = %d\n", errno);
    ok(time[0] == '\0', "time[0] != '\\0'\n");

    err = p_strtime_s(time, 9);
    ok(err == 0, "err = %x\n", err);
}

static void test_wstrdate(void)
{
    wchar_t date[16], * result;
    int month, day, year, count, len;
    wchar_t format[] = { '%','0','2','d','/','%','0','2','d','/','%','0','2','d',0 };

    result = _wstrdate(date);
    ok(result == date, "Wrong return value\n");
    len = wcslen(date);
    ok(len == 8, "Wrong length: returned %d, should be 8\n", len);
    count = swscanf(date, format, &month, &day, &year);
    ok(count == 3, "Wrong format: count = %d, should be 3\n", count);
}

static void test_wstrtime(void)
{
    wchar_t time[16], * result;
    int hour, minute, second, count, len;
    wchar_t format[] = { '%','0','2','d',':','%','0','2','d',':','%','0','2','d',0 };

    result = _wstrtime(time);
    ok(result == time, "Wrong return value\n");
    len = wcslen(time);
    ok(len == 8, "Wrong length: returned %d, should be 8\n", len);
    count = swscanf(time, format, &hour, &minute, &second);
    ok(count == 3, "Wrong format: count = %d, should be 3\n", count);
}

START_TEST(time)
{
    init();

    test_ctime();
    test_gmtime();
    test_mktime();
    test_localtime();
    test_strdate();
    test_strtime();
    test_wstrdate();
    test_wstrtime();
}
