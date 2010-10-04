

# This application accomplishes (when it's compiled):

  * Connects to MySQL on localhost
  * Auto-Discovers all tables from a db of your choosing
  * Creates models in C for those tables
  * Creates another C application which:
      * Connects to MySQL on localhost
      * loads all the content of your tables into ram (using the previously created C models)
      * provides a basic web interface to access that ram-cached content

## Example:

Say you have a database, called `ttc`
In that database you have 1 table called `routes`

    MYSQL> describe routes;
    +-------------+--------------+------+-----+---------+----------------+ 
    | Field       | Type         | Null | Key | Default | Extra          | 
    +-------------+--------------+------+-----+---------+----------------+ 
    | id          | int(11)      | NO   | PRI | NULL    | auto_increment | 
    | created_at  | datetime     | YES  |     | NULL    |                | 
    | updated_at  | datetime     | YES  |     | NULL    |                | 
    | name        | varchar(100) | YES  |     | NULL    |                | 
    | uri         | varchar(100) | YES  |     | NULL    |                | 
    +-------------+--------------+------+-----+---------+----------------+ 
    6 rows in set (0.01 sec)

This application will produce the following C model:

    struct Route {
      int id;
      char created_at[20];
      char updated_at[20];
      char name[100];
      char uri[100];
    };
    
    struct Route *routes = 0;
    int num_routes = 0;
    int load_routes();

When load_routes() is called, it will load all the MySQL content into those models.
On which you could go `routes[2].name` to access the name of route 2.

Once ./ttc_web_test is created and executed you can 
do a web request to http://localhost:4444/routes?id=2 and it will respond with 
something like (assuming that was what was in your database):

    { 
      "id":2, 
      "created_at":"2007-09-25 19:45:37", 
      "updated_at":"2010-06-15 16:52:20", 
      "name":"Bloor-Danforth Subway", 
      "uri":"bloor-danforth_subway" 
    }

> This isn't meant for data that changes at any decent frequency - you have to restart the application to reload your data. 
> The web interface itself isn't terribly useful, I hope to add more example usage.
> Also, another note: keep your IDs small and together - this app allocates space from 0 to the MAX(id)+1, which means if your
> table has only 1 row but it's ID is 900k, it will allocate space for 900k rows for that single row.  *This is working as intended* 

### the ability to use routes[id] directly is the main reason anything that uses this will be *lickity split fast*.
