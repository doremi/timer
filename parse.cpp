#include <stdio.h>
#include <string.h>
#include <jansson.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <utility>

using namespace std;

#define TIME_LOG_FILE "time.log"

#if JANSSON_VERSION_HEX <= 0x020400
#define json_array_foreach(array, index, value) \
  for(index = 0;                                                        \
      index < json_array_size(array) && (value = json_array_get(array, index)); \
      index++)
#endif

bool is_same_day(struct timeval &a, struct timeval &b)
{
    struct tm t1, t2;
    localtime_r(&a.tv_sec, &t1);
    localtime_r(&b.tv_sec, &t2);

    return (t1.tm_year == t2.tm_year &&
            t1.tm_mon == t2.tm_mon &&
            t1.tm_mday == t2.tm_mday);
}

void build_timeval(int year, int month, int day, int hour, int minute, int second, struct timeval *tv)
{
    struct tm t;
    memset(&t, 0, sizeof(t));
    memset(tv, 0, sizeof(struct timeval));
    t.tm_year = year;
    t.tm_mon = month;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    tv->tv_sec = mktime(&t);
}

TEST(is_same_day_test, HandleNoneZeroInput)
{
    struct timeval a, b;

    // a: 2013/01/02 17:53:00
    // b: 2013/01/02 17:53:00
    build_timeval(2013, 1, 2, 17, 53, 0, &a);
    build_timeval(2013, 1, 2, 17, 53, 0, &b);
    EXPECT_TRUE(is_same_day(a, b));

    // a: 2013/01/01 17:53:00
    // b: 2013/01/02 17:53:00
    build_timeval(2013, 1, 1, 17, 53, 0, &a);
    EXPECT_FALSE(is_same_day(a, b));

    // a: 2013/01/02 17:53:00
    // b: 2013/05/02 17:53:00
    build_timeval(2013, 1, 2, 17, 53, 0, &a);
    build_timeval(2013, 1, 5, 17, 53, 0, &b);
    EXPECT_FALSE(is_same_day(a, b));

    // a: 2013/01/02 17:53:00
    // b: 2012/01/02 17:53:00
    build_timeval(2013, 1, 2, 17, 53, 0, &a);
    build_timeval(2012, 1, 2, 17, 53, 0, &b);
    EXPECT_FALSE(is_same_day(a, b));

    // a: 2013/01/02 17:53:00
    // b: 2013/01/02 16:53:00
    build_timeval(2013, 1, 2, 17, 53, 0, &a);
    build_timeval(2013, 1, 2, 16, 53, 0, &b);
    EXPECT_TRUE(is_same_day(a, b));

    // a: 2013/01/02 17:53:00
    // b: 2013/01/02 17:43:00
    build_timeval(2013, 1, 2, 17, 53, 0, &a);
    build_timeval(2013, 1, 2, 16, 43, 0, &b);
    EXPECT_TRUE(is_same_day(a, b));

    // a: 2013/01/02 17:53:20
    // b: 2013/01/02 17:53:00
    build_timeval(2013, 1, 2, 17, 53, 20, &a);
    build_timeval(2013, 1, 2, 16, 53, 0, &b);
    EXPECT_TRUE(is_same_day(a, b));
}

time_t sum_day(vector<pair<struct timeval, struct timeval>> &v)
{
    time_t total = 0;
    struct timeval res;
    pair<struct timeval, struct timeval> p;

    for (size_t i = 0; i < v.size(); ++i) {
        p = v[i];
        timersub(&p.second, &p.first, &res);
        total += res.tv_sec;
    }
    return total;
}

// partition_day() just use start to split, don't care about end
void partition_day(vector<vector<pair<struct timeval, struct timeval>>> &out, json_t *j_array)
{
    int i = -1;
    size_t index;
    json_t *value;
    struct timeval oldtv, newtv, endtv;

    memset(&oldtv, 0, sizeof(oldtv));
    memset(&newtv, 0, sizeof(newtv));
    memset(&endtv, 0, sizeof(endtv));

    json_array_foreach(j_array, index, value) {
        newtv.tv_sec = json_integer_value(json_object_get(value, "start"));
        endtv.tv_sec = json_integer_value(json_object_get(value, "end"));
        if (!is_same_day(oldtv, newtv)) {
            ++i;
            out.push_back(vector<pair<struct timeval, struct timeval>>());
        }
        out[i].push_back(make_pair(newtv, endtv));
        oldtv = newtv;
    }
}

TEST(partition_day_test, HandleNoneZeroInput)
{
    vector<vector<pair<struct timeval, struct timeval>>> vec;
    json_t *j_array = json_array();
    json_t *record;
    struct timeval tv[5];
    struct timeval tvend[5];

    build_timeval(2013, 1, 1, 5, 18, 0, &tv[0]);
    build_timeval(2013, 1, 1, 5, 19, 10, &tvend[0]);

    build_timeval(2013, 1, 2, 6, 20, 11, &tv[1]);
    build_timeval(2013, 1, 2, 6, 20, 21, &tvend[1]);

    build_timeval(2013, 1, 2, 7, 30, 20, &tv[2]);
    build_timeval(2013, 1, 2, 7, 30, 30, &tvend[2]);

    build_timeval(2013, 1, 2, 1, 10, 0, &tv[3]);
    build_timeval(2013, 1, 2, 1, 10, 10, &tvend[3]);

    build_timeval(2013, 2, 2, 10, 20, 30, &tv[4]); 
    build_timeval(2013, 2, 2, 10, 20, 40, &tvend[4]); 

    for (size_t i = 0; i < sizeof(tv)/sizeof(struct timeval); ++i) {
        record = json_pack("{sisi}", "start", tv[i].tv_sec, "end", tvend[i].tv_sec);
        json_array_append_new(j_array, record);
    }

    partition_day(vec, j_array);

    EXPECT_EQ(3, vec.size());

    EXPECT_EQ(70, sum_day(vec[0]));
    EXPECT_EQ(30, sum_day(vec[1]));
    EXPECT_EQ(10, sum_day(vec[2]));
}

void print_duration(time_t duration)
{
    int h, m, s;
    h = duration / 3600;
    m = duration % 3600 / 60;
    s = duration % 3600 % 60;
    printf("%02d:%02d:%02d\n", h, m, s);
}

void print_detail_day(vector<pair<struct timeval, struct timeval>> &v)
{
    char startbuf[32], endbuf[32];
    struct timeval res;
    pair<struct timeval, struct timeval> p;

    for (size_t i = 0; i < v.size(); ++i) {
        p = v[i];
        timersub(&p.second, &p.first, &res);
        printf("%s%s", ctime_r(&p.first.tv_sec, startbuf), ctime_r(&p.second.tv_sec, endbuf));
        print_duration(res.tv_sec);
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    //    testing::InitGoogleTest(&argc, argv);
    //    return RUN_ALL_TESTS();

    vector<vector<pair<struct timeval, struct timeval>>> vec;
    json_t *j_array = json_load_file(TIME_LOG_FILE, JSON_DECODE_ANY, NULL);

    partition_day(vec, j_array);

    for (size_t i = 0; i < vec.size(); ++i) {
        time_t t = sum_day(vec[i]);
        struct tm res;
        localtime_r(&vec[i][0].first.tv_sec, &res);
        printf("%d/%02d/%02d\n", res.tm_year+1900, res.tm_mon+1, res.tm_mday);
        printf("================\n\n");
        print_detail_day(vec[i]);
        printf("Total: ");
        print_duration(t);
        printf("****************\n\n");
    }

    return 0;
}
