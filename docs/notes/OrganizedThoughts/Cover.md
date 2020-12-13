# SR Coding Notes

This is just a document of random notes on computer science for my own reference.
Could be useful to someone else but it's not meant for that.

[Error handling](ErrorHandling.md)


/*
search for "serialization"
same thing
fwrite whole struct is fine, but that prevents you to update/patch game with changed struct contents, so usually you write some kind of header with version, and then have different code paths for different versions (if necessary to support older saves)
also depending on game you might want to validate values loaded back, to prevent buffer overflows and other memory issues when somebody messes up with bytes in file (or accidental file corruption)
also never fopen existing save to overwrite with new data. Always create new file, and then rename over old file, if you need to overwrite
this will prevent save file corruption when writing file fails for some reason (only partial amount of bytes written)
*/
