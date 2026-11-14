# Pipeline variants (not built)

These are experimental versions of pipeline stages, kept for reference. The top-level
`CMakeLists.txt` does not build them. Each folder keeps its original `CMakeLists.txt`, which
current CMake refuses (it asks for CMake 2.8) and which looks for a `src/` folder the sources are no
longer in. The two stage-3 variants build with, for example:

```bash
c++ -std=c++11 -O2 StatisBig.cpp -o StatisBig \
    $(pkg-config --cflags --libs opencv5 gsl) -lboost_filesystem
```

(use `opencv4` with OpenCV 4, and add `-I`/`-L` for Boost if it is not on the default paths).

| Folder | Variant of | Notes |
|---|---|---|
| `template_matching_taylor_length/` | stage 2 | Originally `TemplateMatching-Taylor`. It is the thesis-appendix listing plus an unfinished Taylor-length parameter (`const double tau = ;`), so it does not compile; line 59 then reads into that constant |
| `interface_statistics_big_curvature/` | stage 3 | Originally `Statis-Big-Curvature` |
| `interface_statistics_moving_average_only/` | stage 3 | Moving-average smoothing only, no B-spline |
