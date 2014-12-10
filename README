libzip++ -- A safe modern C++ wrapper around libzip
===================================================

Introduction
------------

The libzip library is a good C library for opening and creating zip archives,
this wrapper provides safe C++ classes around this great library.

The benefits:

* Automatic allocations and destructions (RAII)
* Easy way to add files
* Easy API
* Easy file reading in archive
* Convenience thanks to C++ function overloads

Documentation
-------------

The reference API is documented on the [Wiki](https://bitbucket.org/markand/libzip/wiki/Home).

Usage
-----

Very simple extraction of a file.

````cpp
try {
	ZipArchive archive{"mydata.zip"};
	ZipStat stat = archive.stat("README");
	ZipFile file = archive.open("README");

	std::cout << "content of README:" << std::endl;
	std::cout << file.read(stat.size);
} catch (const std::exception &ex) {
	std::cerr << ex.what() << std::endl;
	std::exit(1);
}
````