sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

sqlrelay:prepareQuery("begin  "
    "select image into :image from images;  "
    "select description into :desc from images;  end;"),
sqlrelay:defineOutputBindBlob("image"),
sqlrelay:defineOutputBindClob("desc"),
sqlrelay:executeQuery(),

Image = sqlrelay:getOutputBindBlob("image"),
ImageLength = sqlrelay:getOutputBindLength("image"),

Desc = sqlrelay:getOutputBindClob("desc"),
DescLength = sqlrelay:getOutputBindLength("desc"),

sqlrelay:endSession(),

... do something with Image and Desc ...
