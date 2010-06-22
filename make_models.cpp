
#include <map>
#include <string>
#include <vector>
#include <mysql.h>

/////////////////////////////
std::string database = "ttc";  // better be able to log in as root with no password
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

std::map<std::string, std::string> pluralize_exceptions;

void populate_pluralize_exceptions()
{
  pluralize_exceptions["branch"] = "branches";
  pluralize_exceptions["vertex"] = "vertices";
}

std::string pluralize(std::string val)
{
  if (pluralize_exceptions.size() == 0)
    populate_pluralize_exceptions();
  
  if (pluralize_exceptions.find(val) != pluralize_exceptions.end()) return pluralize_exceptions[val];
  
  if (val.size() > 2 && val[val.size()-1] != 's') val.push_back('s');
  return val;
}

int main(int argc, char **argv, char **envp)
{
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
  
  ///////////////////////////////////////////////////////////////////////////////////////////////
  
  sprintf(query, "models.h");
  fp = fopen(query, "w");
  for (std::vector<std::string>::iterator it = table_names.begin() ; it != table_names.end() ; it++)
  {
    std::string table_name = (*it).c_str();
    if (table_name != "routes") continue; // kbfu
    
    fprintf(fp, "#include \"%s_models/%s.h\"\n", database.c_str(), table_name.c_str());
  }
  fclose(fp);
  
  for (std::vector<std::string>::iterator it = table_names.begin() ; it != table_names.end() ; it++)
  {
    std::string table_name = (*it).c_str();
    std::string class_name = getClassNameFromTableName(table_name);
    
    if (table_name != "stops" && table_name != "locations" && table_name != "routes" && table_name != "shapes" && table_name != "schedules" && table_name != "services" && table_name != "station_edges") continue;
    if (table_name != "routes") continue; // kbfu
    
    sprintf(query, "%s_models/%s.h", database.c_str(), table_name.c_str());
    fp = fopen(query, "w");
    fprintf(fp, "\n#ifndef POW_%s_%s_H\n#define POW_%s_%s_H\n\n/* This file was generated, so it's probably best not to expect edits to stick */\n\nstruct %s {\n  int id;\n", database.c_str(), table_name.c_str(), database.c_str(), table_name.c_str(), class_name.c_str());
    
    for (unsigned int i = 0 ; i < tables[table_name].fields.size() ; i++)
    {
      sField *f = &tables[table_name].fields[i];
      fprintf(fp, "  %s %s%s;\n", f->type.c_str(), f->name.c_str(), f->size==0?"":"[50]");
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
    
    fprintf(fp, "\nextern struct %s *%s;\nextern int num_%s;\n\nextern int load_%s();\n\n#endif", class_name.c_str(), table_name.c_str(), table_name.c_str(), table_name.c_str());
    fclose(fp);
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    sprintf(query, "%s_models/%s.c", database.c_str(), table_name.c_str());
    fp = fopen(query, "w");
    fprintf(fp, "\n#include \"%s.h\"\n"
                "#include <mysql.h>\n"
                "#include <stdlib.h>\n"
                "#include <string.h>\n"
                "#include <stdio.h>\n", table_name.c_str());
    
    /*for (unsigned int i = 0 ; i < tables[table_name].assocs.size() ; i++)
    {
      sAssoc *a = &tables[table_name].assocs[i];
      if (a->type == HAS_ONE)
        fprintf(fp, "-(id)get%s {\n  \n};\n", getClassNameFromTableName(a->table_name).c_str());
      else if (a->type == HAS_MANY)
        fprintf(fp, "-(id)get%s {\n  \n};\n", pluralize(getClassNameFromTableName(a->table_name)).c_str());
    }*/
    
    fprintf(fp, "\n/* This file was generated, so it's probably best not to expect edits to stick */\n\n"
                "struct %s *%s = 0;\nint num_%s = 0;\n\n",  class_name.c_str(), table_name.c_str(), table_name.c_str());
    
    fprintf(fp, "int load_%s()\n{\n", table_name.c_str());
    
    fprintf(fp, "  MYSQL mysql;\n" 
                "  MYSQL_ROW row;\n" 
                "  MYSQL_RES * res;\n" 
                "  char query[500];\n\n");
    
    fprintf(fp, "  if ((mysql_init(&mysql) == NULL))\n"
                "    return printf(\"mysql_init error\\n\");\n"
                "  if (!mysql_real_connect(&mysql, \"localhost\", \"root\", \"\", \"%s\", 0, NULL, 0))\n"
                "    return printf(\"mysql_real_connect error (%%s)\\n\", mysql_error(&mysql));\n\n", database.c_str());
    
    fprintf(fp, "  if (mysql_query(&mysql, \"SELECT id, ");
    for (unsigned int i = 0 ; i < tables[table_name].fields.size() ; i++)
    {
      sField *f = &tables[table_name].fields[i];
      fprintf(fp, "%s%s", f->name.c_str(), (i==tables[table_name].fields.size()-1)?"":", ");
    }
    fprintf(fp, " FROM %s\")==0)\n", table_name.c_str());
    
    fprintf(fp, "  {\n"
  	            "    res = mysql_store_result(&mysql);\n"
                "    num_%s = mysql_num_rows(res);\n"
                "    %s = (struct %s*)realloc(%s, sizeof(%s)*num_%s);\n", table_name.c_str(), table_name.c_str(), class_name.c_str(), table_name.c_str(), class_name.c_str(), table_name.c_str());
    
    fprintf(fp, "    int i = 0;\n"
                "    while((row = mysql_fetch_row(res)))\n"
                "    {\n"
                "      %s[i].id = atoi(row[0]);\n", table_name.c_str());
    for (unsigned int i = 0 ; i < tables[table_name].fields.size() ; i++)
    {
      sField *f = &tables[table_name].fields[i];
      if (f->type == "int")
        fprintf(fp, "      if (row[%d] != NULL) %s[i].%s = atoi(row[%d]);\n", i+1, table_name.c_str(), f->name.c_str(), i+1);
      else if (f->type == "float")
        fprintf(fp, "      if (row[%d] != NULL) %s[i].%s = atof(row[%d]);\n", i+1, table_name.c_str(), f->name.c_str(), i+1);
      else if (f->type == "char")
        fprintf(fp, "      if (row[%d] != NULL) strncpy(%s[i].%s, row[%d], sizeof(%s[i].%s));\n", i+1, table_name.c_str(), f->name.c_str(), i+1, table_name.c_str(), f->name.c_str());
    }
    fprintf(fp, "      i++;\n"
                "    }\n\n"
                "    mysql_free_result(res);\n"
                "  }\n");
    
    fprintf(fp, "}\n");
    fclose(fp);
  }
  
  sprintf(query, "cd %s_models ; g++ *.c -c -I%s", database.c_str(), location_of_mysql_h.c_str());
  system(query);
  
  printf("\ndone\n");
  
  return 0;
}