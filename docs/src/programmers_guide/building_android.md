Building for Android    {#motive_guide_android}
====================

# Prerequisites    {#motive_android_prerequisites}

Set up your build environment for Android builds by following these steps.

   1. Install [Java 1.7][], required to use Android tools.
        * **Windows / OSX**: Download the [Java 1.7][] installer and run it to
          install.
        * **Linux**: on systems that support apt-get...<br/>
          `sudo apt-get install openjdk-7-jdk`

   2. Install the [Android SDK][], required to build Android applications.<br/>
      [Android Studio][] is the easiest way to install and configure the latest
      [Android SDK][]. Or you can install the [stand-alone SDK tools][].

   3. Install the [Android NDK][], required to develop Android native (C/C++)
      applications.<br/>
        * Download and unpack the latest version of the [Android NDK][] to a
          directory on your machine.
          * Tested with `android-ndk-r10e`.

   4. Add the [Android SDK][]'s `sdk/tools` directory and the [Android NDK][]
      directory to the [PATH variable][].
        * **Windows**:
          Start -> Control Panel -> System and Security -> System ->
          Advanced system settings -> Environment Variables,
          then select PATH and press `Edit`.
          Add paths similar to the following, adjusted for your install
          locations:
          `c:\Users\me\AppData\Local\Android\sdk\tools;c:\Users\me\android-ndk`
        * **Linux**: if the [Android SDK][] is installed in
          `/home/androiddev/adt` and the [Android NDK][] is
          installed in `/home/androiddev/ndk` the following line should be
          added to `~/.bashrc`.<br/>
          `export PATH="$PATH:/home/androiddev/adt/sdk/tools:/home/androiddev/ndk"`
        * **OS X**: if the [Android SDK][] is installed in
          `~/Library/Android/` and the [Android NDK][] is
          installed in `~/bin/android_ndk-r10e` the following line should be
          added to `~/.bash_profile`.<br/>
          `export PATH=$PATH:~/bin/android_ndk-r10d:~/Library/Android/sdk/tools`

   5. On **Linux**, ensure [Java 1.7][] is selected, in the event multiple Java
      versions are installed on the system.
        * on Ubunutu, run `update-java-alternatives` to select the correct
          Java version.

   6. On **Windows**, set the `JAVA_HOME` variable to the Java install location
        * see [Setting Windows Environment Variables][].
        * set `variable name` to JDK_HOME.
        * set `variable value` to your Java installation directory,
          something like `C:\Program Files\Java\jdk1.7.0_75`.


Additionally, if you'd like to use the handy tools in [fplutil][],
   * [Apache Ant][], required to build Android applications with [fplutil][].
        * **Windows / OS X**: Download the latest version of [Apache Ant][] and
          unpack it to a directory.
        * **Linux**: on systems that support `apt-get`...<br/>
          `sudo apt-get install ant`
        * Add the [Apache Ant][] install directory to the [PATH variable][].
   * [Python 2.7][], required to use [fplutil][] tools.<br/>
        * **Windows / OS X**: Download the latest package from the [Python 2.7][]
          page and run the installer.
        * **Linux**: on a systems that support `apt-get`...<br/>
          `sudo apt-get install python`


# Code Generation    {#motive_guide_android_code_generation}

Using `ndk-build`:

~~~{.sh}
    cd motive
    ndk-build
~~~

Using `fplutil`:
~~~{.sh}
    cd motive
    ./dependencies/fplutil/bin/build_all_android -E dependencies -T debug -i -r
~~~

*Note: The `-T debug`, `-i`, and `-r` flags are optional. The `-T` flag sets up
debug signing of the app, which allows it to be installed for testing purposes.
The `-i` flag declares that the script should install the app to a connected
device. The `-r` flag declares that the app should also be run, after it is
installed.*


# The Benchmarker App   {#motive_guide_android_benchmarker_app}

The `Benchmarker` app is in motive/src/benchmarker. It creates and updates 40000
[Motivators][] of various flavors, and measures their various execution times.

To install and run the `Benchmarker` app on an Android device:

   * Install the [Prerequisites](@ref motive_android_prerequisites).
   * Attach an Android device for [USB debugging][].
   * Open a command line window and go to motive/benchmarker.
   * Execute `motive/dependencies/fplutil/bin/build_all_android` with the
     `-S` (app signing), `-i` (install) and `-r` (run) options.

~~~{.sh}
    cd motive/src/benchmarker
    ../../dependencies/fplutil/bin/build_all_android -E dependencies -S -i -r
~~~


<br>

  [adb]: http://developer.android.com/tools/help/adb.html
  [ADT]: http://developer.android.com/tools/sdk/eclipse-adt.html
  [Android Developer Tools]: http://developer.android.com/sdk/index.html
  [Android NDK]: http://developer.android.com/tools/sdk/ndk/index.html
  [Android SDK]: http://developer.android.com/sdk/index.html
  [Android Studio]: http://developer.android.com/sdk/index.html
  [Apache Ant]: https://www.apache.org/dist/ant/binaries/
  [apk]: http://en.wikipedia.org/wiki/Android_application_package
  [fplutil]: http://google.github.io/fplutil
  [fplutil prerequisites]: http://google.github.io/fplutil/fplutil_prerequisites.html
  [Java 1.7]: http://www.oracle.com/technetwork/java/javase/downloads/jdk7-downloads-1880260.html
  [managing avds]: http://developer.android.com/tools/devices/managing-avds.html
  [Motivators]: @ref motive_guide_motivators
  [NDK Eclipse plugin]: http://developer.android.com/sdk/index.html
  [PATH variable]: http://en.wikipedia.org/wiki/PATH_(variable)
  [Python 2.7]: https://www.python.org/download/releases/2.7/
  [Setting Windows Environment Variables]: http://www.computerhope.com/issues/ch000549.htm
  [stand-alone SDK tools]: http://developer.android.com/sdk/installing/index.html?pkg=tools
  [USB debugging]: http://developer.android.com/tools/device.html
