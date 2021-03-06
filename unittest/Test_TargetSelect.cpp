#include "TestHarness.hpp"
#include "TargetSelect.hpp"
#include "Common.hpp"

#include <algorithm>

using namespace t2;

static const char* s_Configs[] = { "win32-msvc", "macosx-clang", "linux-gcc" };
static const char* s_Variants[] = { "debug", "production", "release" };
static const char* s_SubVariants[] = { "default", "special" };

static bool RunTest(
  const char** names, int name_count,
  const TargetSpec* expected_specs, size_t expected_spec_count, 
  const char** expected_names, size_t expected_name_count)
{
  MemAllocHeap heap;
  HeapInit(&heap, MB(1), HeapFlags::kDefault);

  uint32_t config_hashes[ARRAY_SIZE(s_Configs)]         = { Djb2Hash(s_Configs[0]), Djb2Hash(s_Configs[1]), Djb2Hash(s_Configs[2]) };
  uint32_t variant_hashes[ARRAY_SIZE(s_Variants)]       = { Djb2Hash(s_Variants[0]), Djb2Hash(s_Variants[1]), Djb2Hash(s_Variants[2]) };
  uint32_t subvariant_hashes[ARRAY_SIZE(s_SubVariants)] = { Djb2Hash(s_SubVariants[0]), Djb2Hash(s_SubVariants[1]) };

  TargetSelectInput input;
  input.m_ConfigCount = ARRAY_SIZE(config_hashes);
  input.m_VariantCount = ARRAY_SIZE(variant_hashes);
  input.m_SubVariantCount = ARRAY_SIZE(subvariant_hashes);
  input.m_ConfigNameHashes = config_hashes;
  input.m_VariantNameHashes = variant_hashes;
  input.m_SubVariantNameHashes = subvariant_hashes;

  input.m_InputNameCount = name_count;
  input.m_InputNames = names;

  input.m_DefaultConfigIndex = 0;
  input.m_DefaultVariantIndex = 0;
  input.m_DefaultSubVariantIndex = 0;

  Buffer<TargetSpec> specs;
  Buffer<const char*> target_names;

  BufferInit(&specs);
  BufferInit(&target_names);

  SelectTargets(input, &heap, &specs, &target_names);

  std::sort(specs.m_Storage, specs.m_Storage + specs.m_Size);
  std::sort(target_names.m_Storage, target_names.m_Storage + target_names.m_Size, [](const char* l, const char* r) { return strcmp(l, r) < 0; });

  bool success = false;

  if (specs.m_Size == expected_spec_count)
  {
    if (0 == memcmp(specs.m_Storage, expected_specs, sizeof(TargetSpec) * expected_spec_count))
    {
      if (target_names.m_Size == expected_name_count)
      {
        size_t i = 0;
        for ( ; i < expected_name_count; ++i)
        {
          if (0 != strcmp(expected_names[i], target_names[i]))
          {
            break;
          }
        }

        if (i == expected_name_count)
        {
          success = true;
        }
      }
    }
  }

  BufferDestroy(&target_names, &heap);
  BufferDestroy(&specs, &heap);

  HeapDestroy(&heap);

  return success;
}

BEGIN_TEST_CASE("TargetSelect/Empty")
{
  const TargetSpec specs[] = { { 0, 0, 0 } };
  ASSERT_EQUAL(RunTest(nullptr, 0, specs, ARRAY_SIZE(specs), nullptr, 0), true);
}
END_TEST_CASE

BEGIN_TEST_CASE("TargetSelect/RedundantDefault")
{
  const TargetSpec specs[] = { { 0, 0, 0 } };
  const char* names[] = { "win32-msvc" };
  ASSERT_EQUAL(RunTest(names, ARRAY_SIZE(names), specs, ARRAY_SIZE(specs), nullptr, 0), true);
}
END_TEST_CASE

BEGIN_TEST_CASE("TargetSelect/TargetNames")
{
  const TargetSpec specs[] = { { 0, 0, 0 } };
  const char* names[] = { "foo-bar", "some_string" };
  const char* expected_names[] = { "foo-bar", "some_string" };
  ASSERT_EQUAL(RunTest(names, ARRAY_SIZE(names), specs, ARRAY_SIZE(specs), expected_names, ARRAY_SIZE(expected_names)), true);
}
END_TEST_CASE

BEGIN_TEST_CASE("TargetSelect/TwoConfigs")
{
  const TargetSpec specs[] = { { 0, 0, 0 }, { 2, 0, 0 } };
  const char* names[] = { "win32-msvc", "linux-gcc" };
  ASSERT_EQUAL(RunTest(names, ARRAY_SIZE(names), specs, ARRAY_SIZE(specs), nullptr, 0), true);
}
END_TEST_CASE

BEGIN_TEST_CASE("TargetSelect/TwoVariants")
{
  const TargetSpec specs[] = { { 0, 0, 0 }, { 0, 2, 0 } };
  const char* names[] = { "debug", "release" };
  ASSERT_EQUAL(RunTest(names, ARRAY_SIZE(names), specs, ARRAY_SIZE(specs), nullptr, 0), true);
}
END_TEST_CASE

BEGIN_TEST_CASE("TargetSelect/TwoConfigsTwoVariants")
{
  const TargetSpec specs[] = { { 0, 0, 0 }, { 0, 2, 0 }, { 2, 0, 0}, { 2, 2, 0 } };
  const char* names[] = { "debug",  "linux-gcc", "release", "win32-msvc" };
  ASSERT_EQUAL(RunTest(names, ARRAY_SIZE(names), specs, ARRAY_SIZE(specs), nullptr, 0), true);
}
END_TEST_CASE

BEGIN_TEST_CASE("TargetSelect/Specific1")
{
  const TargetSpec specs[] = { { 2, 1, 0 } };
  const char* names[] = { "linux-gcc-production" };
  ASSERT_EQUAL(RunTest(names, ARRAY_SIZE(names), specs, ARRAY_SIZE(specs), nullptr, 0), true);
}
END_TEST_CASE

BEGIN_TEST_CASE("TargetSelect/Specific2")
{
  const TargetSpec specs[] = { { 2, 1, 1 } };
  const char* names[] = { "linux-gcc-production-special" };
  ASSERT_EQUAL(RunTest(names, ARRAY_SIZE(names), specs, ARRAY_SIZE(specs), nullptr, 0), true);
}
END_TEST_CASE

BEGIN_TEST_CASE("TargetSelect/SpecificPair")
{
  const TargetSpec specs[] = { { 0, 2, 0}, { 2, 1, 1 } };
  const char* names[] = { "linux-gcc-production-special", "win32-msvc-release-default" };
  ASSERT_EQUAL(RunTest(names, ARRAY_SIZE(names), specs, ARRAY_SIZE(specs), nullptr, 0), true);
}
END_TEST_CASE

BEGIN_TEST_CASE("TargetSelect/SubVariantOnly")
{
  const TargetSpec specs[] = { { 0, 0, 1} };
  const char* names[] = { "special" };
  ASSERT_EQUAL(RunTest(names, ARRAY_SIZE(names), specs, ARRAY_SIZE(specs), nullptr, 0), true);
}
END_TEST_CASE

BEGIN_TEST_CASE("TargetSelect/SubVariantAndConfig")
{
  const TargetSpec specs[] = { { 2, 0, 1} };
  const char* names[] = { "linux-gcc", "special" };
  ASSERT_EQUAL(RunTest(names, ARRAY_SIZE(names), specs, ARRAY_SIZE(specs), nullptr, 0), true);
}
END_TEST_CASE

BEGIN_TEST_CASE("TargetSelect/TheMotherLode")
{
  const TargetSpec specs[] = {
    { 1, 0, 0 },
    { 1, 0, 1 },
    { 1, 1, 0 },
    { 1, 1, 1 },
    { 2, 0, 0 },
    { 2, 0, 1 },
    { 2, 1, 0 },
    { 2, 1, 1 }
  };
  const char* names[] = { "random-junk", "linux-gcc", "macosx-clang", "default", "special", "debug", "production", "blah" };
  const char* expected_names[] = { "blah", "random-junk" };
  ASSERT_EQUAL(RunTest(names, ARRAY_SIZE(names), specs, ARRAY_SIZE(specs), expected_names, ARRAY_SIZE(expected_names)), true);
}
END_TEST_CASE
