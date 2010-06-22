
#include <map>
#include <string>
#include <vector>
#include <mysql.h>

/////////////////////////////
std::string database = "ttc";  // better be able to log in as root with no password
/////////////////////////////
std::vector<std::string> only_tables;
std::map<std::string, std::string> pluralize_exceptions;
void populate_only_tables()
{
  // if no tables are provided, all tables are done
  if (database == "lcbo")
  {
    only_tables.push_back("stores");
    only_tables.push_back("products");
  }
  if (database == "ttc")
  {
    only_tables.push_back("agency");
    only_tables.push_back("stops");
    only_tables.push_back("locations");
    only_tables.push_back("routes");
    only_tables.push_back("branches");
    only_tables.push_back("shapes");
    //only_tables.push_back("shape_stops");
    //only_tables.push_back("shape_points");
    only_tables.push_back("schedules");
    only_tables.push_back("services");
    only_tables.push_back("service_exceptions");
    only_tables.push_back("vehicle_types");
    only_tables.push_back("station_edges");
  }
}
void populate_pluralize_exceptions()
{
  pluralize_exceptions["branch"] = "branches";
  pluralize_exceptions["vertex"] = "vertices";
}
/////////////////////////////
std::string location_of_mysql_h = "/usr/local/mysql/include/mysql";
std::string location_of_libmysqlclient = "/usr/local/mysql/lib/mysql";
/////////////////////////////


MYSQL mysql;
MYSQL_ROW row;
MYSQL_RES * res;
char query[500];
int num_rows = 0;

FILE *fp;

struct sField {
  std::string name;
  std::string type;
  bool null;
  int size;
};

enum ASSOC_TYPE { NONE, HAS_ONE, HAS_MANY };

struct sAssoc {
  std::string field_name;
  std::string table_name;
  ASSOC_TYPE type;
};

struct sTable {
  std::string name;
  std::vector<sField> fields;
  std::vector<sAssoc> assocs;
};

std::vector<std::string> table_names;
std::map<std::string, sTable> tables;


/*std::string getTableNameFromClassName(std::string val) // translates User => users, Stop => stops
{
  std::string cc = val;
  cc[0] = tolower(cc[0]);
  if (cc.size() > 2 && cc[cc.size()-1] != 's') cc.push_back('s');
  for (int i = 0 ; i < (int)cc.size() ; i++)
  {
    if (cc[i] >= 65 && cc[i] <= 90)
    {
      cc[i] = tolower(cc[i]);
      cc.insert(i, "_");
    }
  }
  return cc;
}*/

std::string getClassNameFromTableName(std::string val) // translates users => User, stops => Stop
{
  std::string cc = val;
  cc[0] = toupper(cc[0]);
  if (cc.size() > 2 && cc[cc.size()-1] == 's' && cc[cc.size()-2] != 's') cc.resize(cc.size()-1);
  for (int i = 0 ; i < (int)cc.size() ; i++)
  {
    if (cc[i] == '_')
    {
      cc.erase(i, 1);
      cc[i] = toupper(cc[i]);
    }
  }
  return cc;
}

int invalid_table(std::string val)
{
  if (only_tables.size() == 0) return 0;
  for (std::vector<std::string>::iterator it = only_tables.begin() ; it != only_tables.end() ; it++)
    if (*it == val) return 0;
  return 1;
}

std::string pluralize(std::string val)
{
  if (pluralize_exceptions.find(val) != pluralize_exceptions.end()) return pluralize_exceptions[val];
  
  if (val.size() > 2 && val[val.size()-1] != 's') val.push_back('s');
  return val;
}

int main(int argc, char **argv, char **envp)
{
  populate_only_tables();
  populate_pluralize_exceptions();
  
  if ((mysql_init(&mysql) == NULL))
    return printf("mysql_init error\n");
  if (!mysql_real_connect(&mysql, "localhost", "root", "", database.c_str(), 0, NULL, 0))
    return printf("mysql_real_connect error (%s)\n", mysql_error(&mysql));
  
  if (database == "")
  {
    printf("You need to pick a database:\n");
    if (mysql_query(&mysql, "SHOW DATABASES")!=0) { printf("mysql_query error (%s)\n", mysql_error(&mysql)); return 0; }
    res = mysql_store_result(&mysql);
    num_rows = mysql_num_rows(res);
    printf("%d databases\n", num_rows);
    while ((row = mysql_fetch_row(res)))
      printf(" %s \n", row[0]);
    mysql_free_result(res);
    return 0;
  }
  
  if (mysql_query(&mysql, "SHOW TABLES")!=0) { printf("mysql_query error (%s)\n", mysql_error(&mysql)); return 0; }
  
	res = mysql_store_result(&mysql);
  num_rows = mysql_num_rows(res);
  printf("%d tables in '%s'\n", num_rows, database.c_str());
  
  sprintf(query, "mkdir -p %s_models ; cd %s_models ; rm *", database.c_str(), database.c_str());
  printf("%s\n", query);
  system(query);
  
  while ((row = mysql_fetch_row(res)))
  {
    tables[row[0]].name = row[0];
    table_names.push_back(row[0]);
  }
  mysql_free_result(res);
  
  for (std::vector<std::string>::iterator it = table_names.begin() ; it != table_names.end() ; it++)
  {
    std::string table_name = (*it).c_str();
    if (invalid_table(table_name)) continue;
    
    std::string class_name = getClassNameFromTableName(table_name);
    
    sprintf(query, "DESCRIBE %s", table_name.c_str());
    if (mysql_query(&mysql, query)!=0) { printf("mysql_query error (%s)\n", mysql_error(&mysql)); return 0; }
    
	  res = mysql_store_result(&mysql);
    while ((row = mysql_fetch_row(res)))
    {
      if (strcmp(row[0],"id")==0) continue;
      sField field;
      field.name = row[0];
      field.null = strcmp(row[2],"YES")==0;
      
      if (row[1][0] == 'i')
      {
        field.type = "int";
        field.size = 0;
      }
      else if (row[1][0] == 'f')
      {
        field.type = "float";
        field.size = 0;
      }
      else if (row[1][0] == 'v' && strlen(row[1]) > 8)
      {
        field.type = "char";
        field.size = atoi(&row[1][8]);
      }
      else if (strcmp(row[1],"text")==0)
      {
        field.type = "char";
        field.size = 0; // will find max(length(.)) later, can't do it here
      }
      else if (strcmp(row[1],"datetime")==0)
      {
        field.type = "char";
        field.size = 20;
      }
      else
      {
        field.type = "char";
        field.size = 50;
      }
      
      tables[(*it).c_str()].fields.push_back(field);
      
      if (field.name.size() > 4 && field.name[field.name.size()-3] == '_' && field.name[field.name.size()-2] == 'i' && field.name[field.name.size()-1] == 'd')
      {
        std::string join_table_name = field.name;
        join_table_name.resize(join_table_name.size() - 3);
        join_table_name = pluralize(join_table_name);
        
        if (tables.find(join_table_name) != tables.end())
        {
          sAssoc assoc;
          assoc.field_name = field.name;
          assoc.type = HAS_ONE;
          assoc.table_name = join_table_name;
          tables[(*it).c_str()].assocs.push_back(assoc);
          
          assoc.field_name = field.name;
          assoc.type = HAS_MANY;
          assoc.table_name = (*it).c_str();
          tables[join_table_name].assocs.push_back(assoc);
        }
        //else
        //  printf("WARNING %s.%s found but table %s does not exist\n", table_name.c_str(), field.name.c_str(), join_table_name.c_str());
      }
    }
    mysql_free_result(res);
  }
  
  for (std::map<std::string, sTable>::iterator it = tables.begin() ; it != tables.end() ; it++)
  {
    sTable *table = &it->second;
    if (invalid_table(table->name)) continue;
    
    for (std::vector<sField>::iterator field = table->fields.begin() ; field != table->fields.end() ; field++)
    {
      
      if (field->type != "char" || field->size != 0) continue;
      sprintf(query, "SELECT MAX(LENGTH(`%s`))+1 FROM %s", field->name.c_str(), table->name.c_str());
      if (mysql_query(&mysql, query)==0)
      {
        MYSQL_RES *res_temp = mysql_store_result(&mysql);
        MYSQL_ROW row_temp = mysql_fetch_row(res_temp);
        
        if (row_temp[0] != NULL)
          field->size = atoi(row_temp[0]);
        else
        {
          printf(" table '%s' has no rows, %s undefined length (text field - assuming 100 chars)\n", table->name.c_str(), field->name.c_str());
          field->size = 100;
        }
        mysql_free_result(res_temp);
      }
      else
      {
        printf("MySQL error for '%s.%s': %s\n", table->name.c_str(), field->name.c_str(), mysql_error(&mysql));
      }
    }
  }
  
  ///////////////////////////////////////////////////////////////////////////////////////////////
  
  sprintf(query, "%s_models.h", database.c_str());
  fp = fopen(query, "w");
  fprintf(fp, "\n/* This file was generated, so it's probably best not to expect edits to stick. \n"
              "  Also, this isn't a regular header file - this is intended to be executed.  */\n\n");
  for (std::vector<std::string>::iterator it = table_names.begin() ; it != table_names.end() ; it++)
  {
    std::string table_name = (*it).c_str();
    if (invalid_table(table_name)) continue;
    
    fprintf(fp, "#include \"%s_models/%s.h\"\n", database.c_str(), table_name.c_str());
  }
  fclose(fp);
  
  for (std::vector<std::string>::iterator it = table_names.begin() ; it != table_names.end() ; it++)
  {
    std::string table_name = (*it).c_str();
    if (invalid_table(table_name)) continue;
    
    std::string class_name = getClassNameFromTableName(table_name);
    
    sprintf(query, "%s_models/%s.h", database.c_str(), table_name.c_str());
    fp = fopen(query, "w");
    fprintf(fp, "\n#ifndef POW_%s_%s_H\n"
                "#define POW_%s_%s_H\n\n"
                "#ifdef __cplusplus\n"
                "extern \"C\" {\n"
                "#endif\n\n"
                "/* This file was generated, so it's probably best not to expect edits to stick */\n\n"
                "struct %s {\n  int id;\n", database.c_str(), table_name.c_str(), database.c_str(), table_name.c_str(), class_name.c_str());
    
    for (unsigned int i = 0 ; i < tables[table_name].fields.size() ; i++)
    {
      sField *f = &tables[table_name].fields[i];
      
      if (f->type == "char" && f->size == 0)
        fprintf(fp, "  %s %s[50]; /* invalid guessed length */\n", f->type.c_str(), f->name.c_str());
      else if (f->type == "char")
        fprintf(fp, "  %s %s[%d];\n", f->type.c_str(), f->name.c_str(), f->size);
      else
        fprintf(fp, "  %s %s;\n", f->type.c_str(), f->name.c_str());
    }
    
    fprintf(fp, "};\n");
    
    /*for (unsigned int i = 0 ; i < tables[table_name].assocs.size() ; i++)
    {
      sAssoc *a = &tables[table_name].assocs[i];
      if (a->type == HAS_ONE)
        fprintf(fp, "struct %s get%s();\n", a->table_name.c_str(), getClassNameFromTableName(a->table_name).c_str());
      else if (a->type == HAS_MANY)
        fprintf(fp, "struct *%s get%s();\n", a->table_name.c_str(), pluralize(getClassNameFromTableName(a->table_name)).c_str());
    }*/
    
    fprintf(fp, "\n"
                "extern struct %s *%s;\n"
                "extern int num_%s;\n\n"
                "extern int load_%s();\n"
                "extern void %s_json_get_request(struct mg_connection *conn, const struct mg_request_info *ri, void *data);\n\n"
                "#ifdef __cplusplus\n"
                "}\n"
                "#endif\n\n"
                "#endif\n", class_name.c_str(), table_name.c_str(), table_name.c_str(), table_name.c_str(), table_name.c_str());
    fclose(fp);
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    sprintf(query, "%s_models/%s.c", database.c_str(), table_name.c_str());
    fp = fopen(query, "w");
    fprintf(fp, "\n#include \"%s.h\"\n"
                "#include <mysql.h>\n"
                "#include <stdlib.h>\n"
                "#include <string.h>\n"
                "#include <stdio.h>\n"
                "#include \"../mongoose.h\"\n\n", table_name.c_str());
    
    /*for (unsigned int i = 0 ; i < tables[table_name].assocs.size() ; i++)
    {
      sAssoc *a = &tables[table_name].assocs[i];
      if (a->type == HAS_ONE)
        fprintf(fp, "-(id)get%s {\n  \n};\n", getClassNameFromTableName(a->table_name).c_str());
      else if (a->type == HAS_MANY)
        fprintf(fp, "-(id)get%s {\n  \n};\n", pluralize(getClassNameFromTableName(a->table_name)).c_str());
    }*/
    
    fprintf(fp, "/* This file was generated, so it's probably best not to expect edits to stick */\n\n"
                "struct %s *%s = 0;\nint num_%s = 0;\n\n",  class_name.c_str(), table_name.c_str(), table_name.c_str());
    
    fprintf(fp, "int load_%s()\n{\n", table_name.c_str());
    
    fprintf(fp, "  MYSQL mysql;\n" 
                "  MYSQL_ROW row;\n" 
                "  MYSQL_RES * res;\n" 
                "  char query[500];\n"
                "  int num_rows = 0;\n"
                "  \n");
    
    fprintf(fp, "  if ((mysql_init(&mysql) == NULL))\n"
                "    return printf(\"mysql_init error\\n\");\n"
                "  if (!mysql_real_connect(&mysql, \"localhost\", \"root\", \"\", \"%s\", 0, NULL, 0))\n"
                "    return printf(\"mysql_real_connect error (%%s)\\n\", mysql_error(&mysql));\n  \n", database.c_str());
    
    fprintf(fp, "  if (mysql_query(&mysql, \"SELECT MAX(id)+1 FROM %s\")==0)\n"
                "  {\n"
                "    res = mysql_store_result(&mysql);\n"
                "    row = mysql_fetch_row(res);\n"
                "    if (row[0] != NULL) num_%s = atoi(row[0]);\n"
                "    mysql_free_result(res);\n"
                "  }\n"
                "  else\n"
                "    printf(\"MySQL error for '%s': %%s\\n\", mysql_error(&mysql));\n\n", table_name.c_str(), table_name.c_str(), table_name.c_str());
    
    fprintf(fp, "  if (mysql_query(&mysql, \"SELECT `id`, ");
    for (unsigned int i = 0 ; i < tables[table_name].fields.size() ; i++)
    {
      sField *f = &tables[table_name].fields[i];
      fprintf(fp, "`%s`%s", f->name.c_str(), (i==tables[table_name].fields.size()-1)?"":", ");
    }
    fprintf(fp, " FROM %s\")==0)\n", table_name.c_str());
    
    fprintf(fp, "  {\n"
  	            "    res = mysql_store_result(&mysql);\n"
                "    num_rows = mysql_num_rows(res);\n"
                "    if (num_%s > 100 && num_%s > num_rows*1.5)\n"
                "      printf(\"Possible problem with the ids on table '%s' (num_rows=%%d, max_id=%%d)\\n\", num_rows, num_%s);\n\n", table_name.c_str(), table_name.c_str(), table_name.c_str(), table_name.c_str());
    
    fprintf(fp, "    %s = (struct %s*)realloc(%s, sizeof(%s)*num_%s);\n", table_name.c_str(), class_name.c_str(), table_name.c_str(), class_name.c_str(), table_name.c_str());
    
    fprintf(fp, "    memset(%s, 0, sizeof(%s)*num_%s);\n"
                "    int i = 0;\n"
                "    while((row = mysql_fetch_row(res)))\n"
                "    {\n"
                "      int id = atoi(row[0]);\n"
                "      %s[id].id = id;\n", table_name.c_str(), class_name.c_str(), table_name.c_str(), table_name.c_str());
    
    for (unsigned int i = 0 ; i < tables[table_name].fields.size() ; i++)
    {
      sField *f = &tables[table_name].fields[i];
      if (f->type == "int")
        fprintf(fp, "      if (row[%d] != NULL) %s[id].%s = atoi(row[%d]);\n", i+1, table_name.c_str(), f->name.c_str(), i+1);
      else if (f->type == "float")
        fprintf(fp, "      if (row[%d] != NULL) %s[id].%s = atof(row[%d]);\n", i+1, table_name.c_str(), f->name.c_str(), i+1);
      else if (f->type == "char")
        fprintf(fp, "      if (row[%d] != NULL) strncpy(%s[id].%s, row[%d], sizeof(%s[id].%s));\n", i+1, table_name.c_str(), f->name.c_str(), i+1, table_name.c_str(), f->name.c_str());
      else
        fprintf(fp, "      /* not sure what to do with %s of type '%s' */\n", f->name.c_str(), f->type.c_str());
    }
    fprintf(fp, "      i++;\n"
                "    }\n"
                "    mysql_free_result(res);\n"
                "  }\n"
                "  else\n"
                "    printf(\"MySQL error for '%s': %%s\\n\", mysql_error(&mysql));\n\n"
                "  printf(\"loaded %%d/%%d %s\\n\", num_rows, num_%s);\n"
                "}\n\n", table_name.c_str(), table_name.c_str(), table_name.c_str());
    
    ///////////////////////////////////////////////////
    
    fprintf(fp, "void %s_json_get_request(struct mg_connection *conn, const struct mg_request_info *ri, void *data)\n"
                "{\n"
                "  mg_printf(conn, \"HTTP/1.1 200 OK\\r\\nContent-Type: text/javascript\\r\\nConnection: close\\r\\n\\r\\n\");\n"
                "  char *id_c = mg_get_var(conn, \"id\");\n"
                "  if (id_c == NULL) { mg_printf(conn, \"{\\\"error\\\":\\\"You must provide an 'id'\\\"}\"); return; }\n"
                "  int id = atoi(id_c);\n"
                "  mg_free(id_c);\n"
                "  if (id >= num_%s || id <= 0) { mg_printf(conn, \"{\\\"error\\\":\\\"Invalid 'id' provided\\\"}\"); return; }\n\n"
                , table_name.c_str(), table_name.c_str());
    
    for (unsigned int i = 0 ; i < tables[table_name].fields.size() ; i++)
    {
      sField *f = &tables[table_name].fields[i];
      if (f->type == "char")
      {
        fprintf(fp, "  char %s_%s[%d];\n", table_name.c_str(), f->name.c_str(), f->size*2+1);
        fprintf(fp, "  strncpy(%s_%s, %s[id].%s, %d);\n", table_name.c_str(), f->name.c_str(), table_name.c_str(), f->name.c_str(), f->size*2+1);
        fprintf(fp, "  for (unsigned int i = 0 ; i < sizeof(%s_%s) ; i++)\n", table_name.c_str(), f->name.c_str());
        fprintf(fp, "    if (%s_%s[i] == '\\\"' || %s_%s[i] == '\\\\')\n", table_name.c_str(), f->name.c_str(),  table_name.c_str(), f->name.c_str());
        fprintf(fp, "      { memmove(&%s_%s[i+1], &%s_%s[i], %d-i); %s_%s[i] = '\\\\'; i++; }\n\n", table_name.c_str(), f->name.c_str(), table_name.c_str(), f->name.c_str(), f->size*2, table_name.c_str(), f->name.c_str());
      }
    }
    
    fprintf(fp, "  mg_printf(conn, \n"
                "    \"{  \\n\"\n"
                "    \"  \\\"id\\\":%%d,\\n\"\n");
    for (unsigned int i = 0 ; i < tables[table_name].fields.size() ; i++)
    {
      sField *f = &tables[table_name].fields[i];
      if (f->type == "int")
        fprintf(fp, "    \"  \\\"%s\\\":%%d%s\\n\"\n", f->name.c_str(), ((i==tables[table_name].fields.size()-1)?"":", "));
      else if (f->type == "float")
        fprintf(fp, "    \"  \\\"%s\\\":%%f%s\\n\"\n", f->name.c_str(), ((i==tables[table_name].fields.size()-1)?"":", "));
      else if (f->type == "char")
        fprintf(fp, "    \"  \\\"%s\\\":\\\"%%s\\\"%s\\n\"\n", f->name.c_str(), ((i==tables[table_name].fields.size()-1)?"":", "));
    }
    fprintf(fp, "    \"}\",\n");
    fprintf(fp, "     %s[id].id,\n", table_name.c_str());
    for (unsigned int i = 0 ; i < tables[table_name].fields.size() ; i++)
    {
      sField *f = &tables[table_name].fields[i];
      if (f->type == "char")
        fprintf(fp, "     %s_%s%s\n", table_name.c_str(), f->name.c_str(), ((i==tables[table_name].fields.size()-1)?"":", "));
      else
        fprintf(fp, "     %s[id].%s%s\n", table_name.c_str(), f->name.c_str(), ((i==tables[table_name].fields.size()-1)?"":", "));
    }
    fprintf(fp, "    );\n");
    
    fprintf(fp, "}\n");
    fclose(fp);
  }
  
  sprintf(query, "%s_web_test.c", database.c_str());
  fp = fopen(query, "w");
  fprintf(fp, "\n#include \"%s_models.h\"\n"
              "#include <stdio.h>\n"
              "#include <stdlib.h>\n"
              "#include <time.h>\n"
              "#include <unistd.h>\n"
              "#include <math.h>\n"
              "#include \"mongoose.h\"\n\n"

              "int main(int argc, char **argv)\n"
              "{\n"
              "  clock_t start = clock();\n"
              "  struct mg_context *ctx = mg_start();\n"
              "  mg_set_option(ctx, \"dir_list\", \"no\");\n"
              "  mg_set_option(ctx, \"ports\", \"4444\");\n  \n"
              , database.c_str());
  
  for (std::vector<std::string>::iterator it = table_names.begin() ; it != table_names.end() ; it++)
  {
    std::string table_name = (*it).c_str();
    if (invalid_table(table_name)) continue;
    
    fprintf(fp, "  load_%s();\n", table_name.c_str());
  }
  fprintf(fp, "  \n");
  
  for (std::vector<std::string>::iterator it = table_names.begin() ; it != table_names.end() ; it++)
  {
    std::string table_name = (*it).c_str();
    if (invalid_table(table_name)) continue;
    
    fprintf(fp, "  mg_set_uri_callback(ctx, \"/%s\", &%s_json_get_request, NULL);\n", table_name.c_str(), table_name.c_str());
  }
  
  fprintf(fp, "  \n"              
              "  printf(\"loaded all models in %%f seconds\\n\", (clock()-(float)start)/CLOCKS_PER_SEC);\n"
              "  printf(\"-------------------------------------------------------------------------\\n\");\n"
              "  for (;;) sleep(10000);\n"
              "  mg_stop(ctx);\n"
              "}");
  fclose(fp);
  
  sprintf(query, "cd %s_models ; g++ *.c -c -I%s ; cd ..", database.c_str(), location_of_mysql_h.c_str());
  printf("%s\n", query);
  system(query);
  
  sprintf(query, "g++ -g mongoose.o %s_models/*.o %s_web_test.c -lmysqlclient -L%s -pg -ldl -lpthread -o %s_web_test", database.c_str(), database.c_str(), location_of_libmysqlclient.c_str(), database.c_str());
  printf("%s\n", query);
  system(query);
  
  printf("\ndone\n");
  
  return 0;
}