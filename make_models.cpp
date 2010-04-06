
#include <map>
#include <string>
#include <vector>
#include <mysql.h>

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


std::string getTableNameFromClassName(std::string val) // translates User => users, Stop => stops
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
}

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
  std::string database = "ttc";
  
  if ((mysql_init(&mysql) == NULL)) { printf("mysql_init error\n"); return 0; }
  if (!mysql_real_connect(&mysql, "localhost", "root", "", database.c_str(), 0, NULL, 0)) { printf("mysql_real_connect error (%s)\n", mysql_error(&mysql)); return 0; }
  
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
  printf("%d tables\n", num_rows);
  
  system("cd models ; rm *");
  
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
    fprintf(fp, "#include \"models/%s.h\"\n", table_name.c_str());
  }
  fclose(fp);
  
  for (std::vector<std::string>::iterator it = table_names.begin() ; it != table_names.end() ; it++)
  {
    std::string table_name = (*it).c_str();
    std::string class_name = getClassNameFromTableName(table_name);
    
    if (table_name != "stops" && table_name != "locations" && table_name != "routes" && table_name != "shapes" && table_name != "schedules" && table_name != "services" && table_name != "station_edges") continue;
    
    sprintf(query, "models/%s.h", table_name.c_str());
    fp = fopen(query, "w");
    fprintf(fp, "@interface %s {\n  int id;\n", class_name.c_str());
    
    for (unsigned int i = 0 ; i < tables[table_name].fields.size() ; i++)
    {
      sField *f = &tables[table_name].fields[i];
      fprintf(fp, "  %s %s%s;\n", f->type.c_str(), f->name.c_str(), f->size==0?"":"[50]");
    }
    
    fprintf(fp, "}\n");
    
    for (unsigned int i = 0 ; i < tables[table_name].assocs.size() ; i++)
    {
      sAssoc *a = &tables[table_name].assocs[i];
      if (a->type == HAS_ONE)
        fprintf(fp, "-(id)get%s;\n", getClassNameFromTableName(a->table_name).c_str());
      else if (a->type == HAS_MANY)
        fprintf(fp, "-(id)get%s;\n", pluralize(getClassNameFromTableName(a->table_name)).c_str());
    }
    
    fprintf(fp, "\n@end");
    fclose(fp);
    
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    sprintf(query, "models/%s.m", table_name.c_str());
    fp = fopen(query, "w");
    fprintf(fp, "#include \"%s.h\"\n@implementation %s\n", table_name.c_str(), class_name.c_str());
  
    for (unsigned int i = 0 ; i < tables[table_name].assocs.size() ; i++)
    {
      sAssoc *a = &tables[table_name].assocs[i];
      if (a->type == HAS_ONE)
        fprintf(fp, "-(id)get%s {\n  \n};\n", getClassNameFromTableName(a->table_name).c_str());
      else if (a->type == HAS_MANY)
        fprintf(fp, "-(id)get%s {\n  \n};\n", pluralize(getClassNameFromTableName(a->table_name)).c_str());
    }
    
    fprintf(fp, "\n@end");
    fclose(fp);
  }
  
  system("cd models ; g++ *.m -c");
  
  printf("\ndone\n");
  
  return 0;
}