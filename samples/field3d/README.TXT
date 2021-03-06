== What this is ==

These field3d tools provide an output driver for .f3d and support for .f3d
files whereever .bgeo is supported.

== Notes on how it works ==

The "name" attribute on the volumes is converted to the "name" attribute
in the field3d file.  If the Houdini volume has a name attribute of
"foo.ext", the f3d field will have name "foo" and attribute "ext".  If there
is no extension, the name and attribute will be the same.

When reading from f3d files the name/attribute will be concatenated with a
"." unless they are the same.

All of the houdini attributes will be copied into metadata of the
same name in the f3d file.  Likewise, any metadata in the f3d file
will become attribues in Houdini.

The original primitive number is stored as the metadata "_houdini_.primnum"
This is used to restore the original primitive order.

The taperx/y values are stored into the "_houdini_.taperx" and 
"_houdini_.tapery"

Any metadata that starts with "_houdini_." isn't converted into
a houdini attribute on load.

== How to build ==

Linux / OSX:
  hcustom -L $HDSO -l Field3D f3dtools.C

Windows:
  hcustom f3dtools.C

If the ROP does not show up, 
setenv HOUDINI_DSO_ERROR 1
and look for any dso errors that are reported.
