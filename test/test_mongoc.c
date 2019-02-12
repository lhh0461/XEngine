#include <bson.h>
#include <bcon.h>
#include <mongoc.h>

    int
main (int   argc,
        char *argv[])
{
    mongoc_client_t      *client;
    mongoc_database_t    *database;
    mongoc_collection_t  *collection;
    bson_t               *command,
                         reply,
                         *insert;
    bson_error_t          error;
    char                 *str;
    bool                  retval;

    /*
     *     * 初始化libmongoc's
     *         */
    mongoc_init ();

    /*
     *     * 创建一个新的client实例
     *         */
    client = mongoc_client_new ("mongodb://localhost:27017");

    /*
     *     * 获取数据库"db_name"和集合"coll_name"的句柄
     *         */
    database = mongoc_client_get_database (client, "db_name");
    collection = mongoc_client_get_collection (client, "db_name", "coll_name");

    /*
     *     * 执行操作。此处以执行ping数据库，以json格式打印结果。并执行一个插入操作。
     *         */

    // 执行命令操作(ping)
    command = BCON_NEW ("ping", BCON_INT32 (1));
    retval = mongoc_client_command_simple (client, "admin", command, NULL, &reply, &error);

    if (!retval) {
        fprintf (stderr, "%s\n", error.message);
        return EXIT_FAILURE;
    }
    // 获取json形式的结果
    str = bson_as_json (&reply, NULL);
    printf ("%s\n", str);    // 打印输出

    // 插入操作命令
    insert = BCON_NEW ("hello", BCON_UTF8 ("world"));

    if (!mongoc_collection_insert (collection, MONGOC_INSERT_NONE, insert, NULL, &error)) {
        fprintf (stderr, "%s\n", error.message);
    }
    mongoc_collection_find_with_opts
    bson_append_utf8

    // 释放资源
    bson_destroy (insert);
    bson_destroy (&reply);
    bson_destroy (command);
    bson_free (str);

    /*
     * 释放拥有的句柄并清理libmongoc
     */
    mongoc_collection_destroy (collection);
    mongoc_database_destroy (database);
    mongoc_client_destroy (client);
    mongoc_cleanup ();

    return 0;
}
