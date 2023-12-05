# Welcome to the JDK!

For build instructions please see the
[online documentation](https://openjdk.java.net/groups/build/doc/building.html),
or either of these files:

- [doc/building.html](doc/building.html) (html version)
- [doc/building.md](doc/building.md) (markdown version)

See <https://openjdk.java.net/> for more information about
the OpenJDK Community and the JDK.

## New GCs
### Full heap parallel mark compact collector
```bash
-Xms32g -Xmx32g -XX:+UseParallelGC -XX:+UseParallelFullMarkCompactGC -XX:NewSize=1k -XX:MaxNewSize=1k -XX:-UseAdaptiveSizePolicy
```
