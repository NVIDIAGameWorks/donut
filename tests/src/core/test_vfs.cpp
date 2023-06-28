/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#include <donut/core/vfs/VFS.h>

#include <donut/tests/utils.h>
#include <filesystem>

using namespace donut;

std::filesystem::path rpath(DONUT_TEST_SOURCE_DIR);

void test_native_filesystem()
{
	vfs::NativeFileSystem fs;

	// folderExists
	{
		CHECK(fs.folderExists(rpath / "CMakeLists.txt") == false);
		CHECK(fs.folderExists(rpath / "src") == true);
		CHECK(fs.folderExists(rpath / "src/core") == true);
		CHECK(fs.folderExists(rpath / "dummy") == false);
	}

	// fileExists
	{
		CHECK(fs.fileExists(rpath / "CMakeLists.txt")==true);
		CHECK(fs.fileExists(rpath / "src/core/test_vfs.cpp") == true);
		CHECK(fs.fileExists(rpath / "dummy") == false);
	}

	// enumerateDirectories
	{
		std::vector<std::string> result;
		CHECK(fs.enumerateDirectories(rpath, vfs::enumerate_to_vector(result), true) == 2);
		CHECK(result.size() == 2);
		CHECK(result[0] == "include");
		CHECK(result[1] == "src");
	}

	// enumerateFiles
	{
		std::vector<std::string> result;
		CHECK(fs.enumerateFiles(rpath, {".txt"}, vfs::enumerate_to_vector(result), true) == 1);
		CHECK(result.size() == 1);
		CHECK(result[0] == "CMakeLists.txt");
	}

	// readFile
	{		
		std::shared_ptr<vfs::IBlob> blob = fs.readFile(rpath / "src/core/test_vfs.cpp");
		CHECK(blob.use_count()>0);
		CHECK(blob->size() > 0);

		std::string data = (char const*)blob->data();
		CHECK(data.find("***HELLO WORLD***")!=std::string::npos);
	}
}

void test_relative_filesystem()
{

	std::shared_ptr<vfs::NativeFileSystem> fs = std::make_shared<vfs::NativeFileSystem>();
	vfs::RelativeFileSystem relativeFS(fs, rpath);

	// folderExists
	{
		CHECK(relativeFS.folderExists("CMakeLists.txt") == false);
		CHECK(relativeFS.folderExists("src") == true);
		CHECK(relativeFS.folderExists("src/core") == true);
		CHECK(relativeFS.folderExists("dummy") == false);
	}

	// fileExists
	{
		CHECK(relativeFS.fileExists("CMakeLists.txt") == true);
		CHECK(relativeFS.fileExists("src/core/test_vfs.cpp") == true);
		CHECK(relativeFS.fileExists(rpath / "CMakeLists.txt") == false);
		CHECK(relativeFS.fileExists("dummy") == false);
	}
	// enumerateDirectories
	{
		std::vector<std::string> result;
		CHECK(relativeFS.enumerateDirectories("/", vfs::enumerate_to_vector(result), true) == 2);
		CHECK(result.size() == 2);
		CHECK(result[0] == "include");
		CHECK(result[1] == "src");
	}
	// enumerateFiles
	{
		std::vector<std::string> result;
		CHECK(relativeFS.enumerateFiles("/", {".txt"}, vfs::enumerate_to_vector(result), true) == 1);
		CHECK(result.size() == 1);
		CHECK(result[0] == "CMakeLists.txt");
	}
	// readFile
	{
		std::shared_ptr<vfs::IBlob> blob = relativeFS.readFile("src/core/test_vfs.cpp");
		CHECK(blob.use_count() > 0);
		CHECK(blob->size() > 0);

		std::string data = (char const*)blob->data();
		CHECK(data.find("***HELLO WORLD***") != std::string::npos);
	}
}

void test_root_filesystem()
{
	vfs::RootFileSystem rootFS;

	CHECK(rootFS.unmount("/foo") == false);

	rootFS.mount("/tests", rpath);

	// folderExists
	{
		CHECK(rootFS.folderExists("/tests/CMakeLists.txt") == false);
		CHECK(rootFS.folderExists("/tests/src") == true);
		CHECK(rootFS.folderExists("/tests/src/core") == true);
		CHECK(rootFS.folderExists("/tests/dummy") == false);
	}

	// fileExists
	{
		CHECK(rootFS.fileExists("/tests/CMakeLists.txt") == true);
		CHECK(rootFS.fileExists("/tests/src/core/test_vfs.cpp") == true);
		CHECK(rootFS.fileExists("/CMakeLists.txt") == false);
		CHECK(rootFS.fileExists("/tests/dummy") == false);
	}
	// enumerateDirectories
	{
		std::vector<std::string> result;
		CHECK(rootFS.enumerateDirectories("/tests", vfs::enumerate_to_vector(result), true) == 2);
		CHECK(result.size() == 2);
		CHECK(result[0] == "include");
		CHECK(result[1] == "src");
	}
	// enumerateFiles
	{
		std::vector<std::string> result;
		CHECK(rootFS.enumerateFiles("/tests", { ".txt" }, vfs::enumerate_to_vector(result), true) == 1);
		CHECK(result.size() == 1);
		CHECK(result[0] == "CMakeLists.txt");
	}
	// readFile
	{
		std::shared_ptr<vfs::IBlob> blob = rootFS.readFile("/tests/src/core/test_vfs.cpp");
		CHECK(blob.use_count() > 0);
		CHECK(blob->size() > 0);

		std::string data = (char const*)blob->data();
		CHECK(data.find("***HELLO WORLD***") != std::string::npos);
	}

	// unmount
	CHECK(rootFS.unmount("/foo") == false);
	CHECK(rootFS.unmount("/tests") == true);
	CHECK(rootFS.unmount("/foo") == false);
}

int main(int, char** argv)
{
	try
	{
		test_native_filesystem();
		test_relative_filesystem();
		test_root_filesystem();
	}
	catch (const std::runtime_error & err)
	{
		fprintf(stderr, "%s", err.what());
		return 1;
	}
	return 0;
}
