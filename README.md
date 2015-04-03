# DashboardImageExtractor (DIE)

Tool for extracting Hansoft dashboards as images.

###Overview
DIE is a command line tool for Windows or OS X for extracting dashboard charts from Hansoft as images. Typically this would be used in order to display dashboard charts on a webpage or similar.

###Terms and conditions
DIE is licensed under a MIT License as stated in the [LICENSE.md].
This tool is not part of the official Hansoft product or subject to its license agreement. The program is provided as is and there is no obligation on Hansoft AB to provide support, update or enhance this program.

By downloading the pre-built packages of DIE you also accept the terms and conditions of the [Hansoft Software Development Kit (.pdf)].

###Building the program
DIE can be built with e.g the freely available [Visual Studio Express 2013 for Desktop (Windows)] or [XCode (OSX)]. 
You will need the [Hansoft SDK] to be able to build the program. Make sure to include the HPMSdkCpp.h and HPMSdkCpp.cpp files in your build directory. You also need to reference the HPMSdk.x64.dll or .dylib.

###Running
A prebuilt version of DIE is also available for [Windows] and [OSX] - Click ‘View raw’ to download. The package includes the following files:
- DashboardImageExtractor
-	An example config file, DIEConfigFile.txt
-	An example ID file, ChartNames.txt
-	HPMResultSetTool, otherwise found in the Hansoft SDK kit
-	HPMSdk.x64.dll or .dylib otherwise found in the Hansoft SDK kit.

Only the HPMSdk.x64 file need to be placed in the run directory of DIE, but for ease of use you can place all files in the same directory. The prebuilt version is tested with Hansoft 8.3 and should work with later versions as well.

DIE requires that you have a Hansoft license with the SDK module active as you use a Hansoft SDK user to extract data. For more information regarding the Hansoft SDK module see [here] or contact Hansoft support: support@hansoft.com.

###Configuration file settings
The configuration file for DIE requires the following settings:
- HansoftServerAddress – This is the DNS or IP to the Hansoft server.
-	HansoftServerPort – This is the port the Hansoft server is listening to.
-	Database – The database you are connecting to.
-	SDKUserName – Name of the SDK user on the database. Note that you need one SDK user per integration used.
-	SDKUserPassword – Password for the SDK user.
-	ChartIDFilePath – Path to the file containing the IDs of the Dashboard pages or charts you want to extract.
-	OutputFolderPath – Path to the directory where you want your extracted charts to be placed.
-	ResultSetToolPath – Path to the HPMResultSetTool
-	OutputResolution – Size of the image output in pixels (width, height), plus text size of the charts (small, medium, large).
-	OutputFormat – Image file format the extracted charts should be generated as. Supports BMP, JPEG, JPG, PBM, PGM, PNG, PPM, XBM, XPM.
-	RefreshRate – The refresh rate of the chart extraction, in seconds.

Each setting must be followed by a ‘=’, immediately followed by the setting value. No spaces are allowed before or after the ‘=’. E.g ‘HansoftServerAddress=localhost‘

Relative paths can be used in the config file, e.g 'OutputFolderPath=./Charts/' to specify a directory 'Charts' located in the DIE run directory on OS X.

See ExampleConfig for a full example.

DIE requires the path to the configuration file as a command line argument when starting, e.g ‘DashboardsImageExtractor C:\DIE\DIEConfigFile.txt’, relative paths can be used.

When running DIE will first create image files of each chart defined in the ChartIDFile, after that it will update those images as determined by the refresh interval.

###ChartIDFile settings
In the ChartFile you specify which charts you want to extract. Input should be one chart or dashboard page ID per line. See the example ChartID file if needed.

Incorrect IDs will be ignored.

The ID of a chart or a page on where the chart lives can be found under detailed chart information of a chart, accessed by clickin the dropdown menu of the chart itself.

If a page ID is entered all charts on that page will be extracted.

###Other
Any software that uses the Hansoft SDK should be run on a machine separate from the one hosting the Hansoft server.

###Support
For questions or problems you may contact drakir.nosslin@gmail.com. Help is provided on availability basis only.

[LICENSE.md]:https://github.com/Hansoft/DashboardImageExtractor/blob/master/LICENSE.md
[Hansoft Software Development Kit (.pdf)]:http://www.hansoft.com/wp-content/uploads/2013/06/SDKLA-final-28-Feb-2013-elektroniskt.pdf
[Visual Studio Express 2013 for Desktop (Windows)]:https://www.visualstudio.com/en-us/downloads/download-visual-studio-vs.aspx
[XCode (OSX)]:https://developer.apple.com/xcode/downloads/
[Hansoft SDK]:http://www.hansoft.com/en/support/downloads/
[OSX]:https://github.com/Hansoft/DashboardImageExtractor/blob/master/DIE%20-%20OSX.zip
[Windows]:https://github.com/Hansoft/DashboardImageExtractor/blob/master/DIE%20-%20Windows.zip
[here]:http://www.hansoft.com/en/support/sdk-integrations
