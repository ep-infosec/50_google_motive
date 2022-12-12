The Motive Animation System
===========================

# Overview    {#motive_overview}

Motive is an animation system written in cross-platform C++.
It's designed to be,
  * **performant** -- data is stored centrally and processed in bulk so that we
    can take advantage of [SIMD] and multi-threading opportunities.
  * **compact** -- spline data is preferred over sampled data, so animations
    requires less memory.
  * **expandable** -- new animation algorithms can be registered and utilized
    without touching central code.
Motive has been tested on [Android], [Windows], [OS X], and [Linux], but
should be portable to any platform that supports C++.

You can see Motive animation in use in the [Zooshi] and [Pie Noon].
sample games.

# Motivation

Static objects may look good in screenshots, but motion is what brings a scene
to life. If done well, even simple motion along one axis can add character and
charm to your game.

The difficulty is in doing the motion well. There is a lot of subtly to motion.
There's a lot that can look ok, but not great, for reasons that are not obvious.

For these two reasons, a simple animation library is valuable--just as valuable,
perhaps, as a simple rendering or audio library.

# Features

The v1.1 release of Motive focusses on speed and scalability. Motive now
supports rigged character animation and blending between those animations.
Motive maintains its aptitude for procedural animation that was the focus of
v1.0.

# Downloading

[Motive] can be downloaded from [GitHub](http://github.com/google/motive) or
the [releases page](http://github.com/google/motive/releases).

**Important**: Motive uses submodules to reference other components it depends
upon so download the source using:

~~~{.sh}
    git clone --recursive https://github.com/google/motive.git
~~~

# Dependencies

Motive depends upon:

* [MathFu][] -- used internally; you can substitute your own [vector][] types
  in the external API.
* [FplUtil][] -- C++ allocator libraries.

Motive has optional features that depend upon:

* [FlatBuffers][] -- allow animation data to be defined in json and then
  compiled to a small binary format.

# Tracking on Google Play

For applications on Google Play that integrate this tool, usage is tracked.
This tracking is done automatically using the embedded version string in
`kVersion`. Aside from consuming a few extra bytes in your application binary,
it shouldn't affect your application at all.  We use this information to let us
know if Motive is useful and if we should continue to invest in it. Since this
is open source, you are free to remove the version string, but we would
appreciate if you would leave it in.

# Feedback and Reporting Bugs

   * Discuss Motive with other developers and users on the
     [Motive Google Group][].
   * File issues on the [Motive Issues Tracker][].
   * Post your questions to [stackoverflow.com][] with a mention of **motive
     animation**.

  [Android]: http://www.android.com
  [Pie Noon]: https://github.com/google/pienoon
  [Zooshi]:  https://github.com/google/zooshi
  [FlatBuffers]: https://github.com/google/flatbuffers
  [FplUtil]: https://github.com/google/fplutil
  [Linux]: http://en.m.wikipedia.org/wiki/Linux
  [MathFu]: https://github.com/google/mathfu
  [Motive Google Group]: http://groups.google.com/group/motive-anim
  [Motive Issues Tracker]: http://github.com/google/motive/issues
  [Motive]: @ref motive_overview
  [OS X]: http://www.apple.com/osx/
  [SIMD]: http://en.wikipedia.org/wiki/SIMD
  [Windows]: http://windows.microsoft.com/
  [stackoverflow.com]: http://stackoverflow.com/search?q=motive+animation
  [vector]: http://en.wikipedia.org/wiki/Euclidean_vector
