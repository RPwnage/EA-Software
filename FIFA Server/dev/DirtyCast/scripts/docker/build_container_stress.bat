@REM generate the version file at ${package.builddir}/pregen_version which is used in the build to generate the final version.c (for more
@REM information see mkver.cs)
CALL nant mkver -D:dirtycast-pregen=true

@REM build dirtycast within docker
CALL docker run %* -it dirtycast/build:active ./build-stress.sh

@REM remove the generated file
CALL nant clean_pregen
