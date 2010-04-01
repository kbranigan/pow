
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

std::string getClassName(std::string val)
{
  std::string cc = val;
  cc[0] = toupper(cc[0]);
  if (cc != "s" && cc[cc.size()-1] == 's' && cc[cc.size()-2] != 's') cc.resize(cc.size()-1);
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

std::vector<std::string> table_names;
std::map<std::string, std::vector<sField> > tables;

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
  
  while ((row = mysql_fetch_row(res)))
    table_names.push_back(row[0]);
  mysql_free_result(res);
  
  for (std::vector<std::string>::iterator it = table_names.begin() ; it != table_names.end() ; it++)
  {
    std::string table_name = (*it).c_str();
    std::string class_name = getClassName(table_name);
    
    if (table_name != "stops" && table_name != "locations") continue;
    
    sprintf(query, "DESCRIBE %s", table_name.c_str());
    printf("%s\n", (*it).c_str());
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
      
      tables[(*it).c_str()].push_back(field);
    }
    
    sprintf(query, "models/%s.h", table_name.c_str());
    fp = fopen(query, "w");
    fprintf(fp, "@interface %s {\n  int id;\n", class_name.c_str());
    
    for (unsigned int i = 0 ; i < tables[table_name].size() ; i++)
    {
      sField *f = &tables[table_name][i];
      fprintf(fp, "  %s %s%s;\n", f->type.c_str(), f->name.c_str(), f->size==0?"":"[50]");
    }
    
    fprintf(fp, "}\n");
    
    for (unsigned int i = 0 ; i < tables[table_name].size() ; i++)
    {
      sField *f = &tables[table_name][i];
      if (f->name.size() > 4 && f->name[f->name.size()-3] == '_' && f->name[f->name.size()-2] == 'i' && f->name[f->name.size()-1] == 'd')
      {
        std::string join_table_name = f->name;
        join_table_name.resize(join_table_name.size()-3);
        join_table_name += 's';
        if (tables.find(join_table_name) != tables.end())
        {
          fprintf(fp, "-(id)get%s;\n", getClassName(join_table_name).c_str());
        }
      }
    }
    
    fprintf(fp, "\n@end");
    fclose(fp);
    
///////////////////////////////////////////////////////////////////////////////////////////////
    
    sprintf(query, "models/%s.m", table_name.c_str());
    fp = fopen(query, "w");
    fprintf(fp, "#include \"%s.h\"\n@implementation %s\n", table_name.c_str(), class_name.c_str());
    
    for (unsigned int i = 0 ; i < tables[table_name].size() ; i++)
    {
      sField *f = &tables[table_name][i];
      if (f->name.size() > 4 && f->name[f->name.size()-3] == '_' && f->name[f->name.size()-2] == 'i' && f->name[f->name.size()-1] == 'd')
      {
        std::string join_table_name = f->name;
        join_table_name.resize(join_table_name.size()-3);
        join_table_name += 's';
        if (tables.find(join_table_name) != tables.end())
        {
          fprintf(fp, "-(id)get%s {\n  \n};\n", getClassName(join_table_name).c_str());
        }
      }
    }
    
    fprintf(fp, "\n@end");
    fclose(fp);
    
    mysql_free_result(res);
  }
  
  
  system("cd models ; g++ *.m -c");
  
  printf("\ndone\n");
  
  return 0;
}