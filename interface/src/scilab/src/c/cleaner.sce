// This file is released into the public domain
// Generated by builder.sce : Please, do not edit this file
// cleaner.sce
// ------------------------------------------------------
curdir = pwd();
cleaner_path = get_file_path('cleaner.sce');
chdir(cleaner_path);
// ------------------------------------------------------
if fileinfo('loader.sce') <> [] then
  mdelete('loader.sce');
end
// ------------------------------------------------------
if fileinfo('libsp_get.so') <> [] then
  mdelete('libsp_get.so');
end
// ------------------------------------------------------
chdir(curdir);
// ------------------------------------------------------
