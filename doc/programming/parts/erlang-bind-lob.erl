sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

sqlrelay:executeQuery("create table images (image blob, description clob)"),

... read an image from a file into ImageData and the length of the
        file into ImageLength ...

... read a description from a file into Description and the length of
        the file into DescLength ...

sqlrelay:prepareQuery("insert into images values (:image,:desc)"),
sqlrelay:inputBindBlob("image", ImageData, ImageLength),
sqlrelay:inputBindClob("desc", Description, DescLength),
sqlrelay:executeQuery(),
