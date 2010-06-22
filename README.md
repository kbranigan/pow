

This application accomplishes (when it's compiled):
  1. Connects to MySQL on localhost
  2. Auto-Discovers all tables from a db of your choosing
  3. Creates models in C for those tables
  4. Creates another C application which:
      I. Connects to MySQL on localhost
      II. loads all the content of your tables into ram (using the previously created C models)
      III. provides a basic web interface to access that ram-cached content

This isn't meant for data that changes at any decent frequency - you have to restart the application to reload your data.
The web interface itself isn't terribly useful, I hope to add more example usage.

Example:

Say you have a database, called 'ttc'
In that database you have 1 table called 'routes'

`MYSQL> describe routes;
+-------------+--------------+------+-----+---------+----------------+
| Field       | Type         | Null | Key | Default | Extra          |
+-------------+--------------+------+-----+---------+----------------+
| id          | int(11)      | NO   | PRI | NULL    | auto_increment | 
| created_at  | datetime     | YES  |     | NULL    |                | 
| updated_at  | datetime     | YES  |     | NULL    |                | 
| name        | varchar(100) | YES  |     | NULL    |                | 
| uri         | varchar(100) | YES  |     | NULL    |                | 
+-------------+--------------+------+-----+---------+----------------+
6 rows in set (0.01 sec)`

This application will produce the following C model:

`struct Route {
  int id;
  char created_at[20];
  char updated_at[20];
  char name[100];
  char uri[100];
  int old_noun_id;
};`

It will also produce:

`struct Route *routes = 0;
int num_routes = 0;
int load_routes();`

When load_routes() is called, it will load all the MySQL content into those models.

Once ./ttc_web_test is created and executed you can 
do a web request to http://localhost:4444/routes?id=2 and it will respond with 
something like (assuming that was what was in your database):

`{  
  "id":2,
  "created_at":"2007-09-25 19:45:37", 
  "updated_at":"2010-06-15 16:52:20", 
  "name":"Bloor-Danforth Subway", 
  "uri":"bloor-danforth_subway"
}`

