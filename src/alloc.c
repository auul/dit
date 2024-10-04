#include <dit.h>

bool dit_alloc_page(Dit_State *P, size_t cap)
{
	Dit_AllocPage *page = malloc(sizeof(Dit_AllocPage) + cap);
	if (!page) {
		return false;
	}

	page->prev = P->mem;
	page->cap  = cap;
	page->at   = 0;
	P->mem     = page;

	return true;
}

void dit_alloc_exit(Dit_State *P)
{
	while (P->mem) {
		Dit_AllocPage *page = P->mem;
		P->mem              = page->prev;
		free(page);
	}
}

void *dit_alloc(Dit_State *P, size_t size)
{
	if (P->mem->cap < P->mem->at + size) {
		size_t re_cap = 2 * P->mem->cap;
		if (re_cap < size) {
			re_cap = size;
		}

		if (!dit_alloc_page(P, re_cap)) {
			return NULL;
		}
	}

	void *ptr = P->mem->raw + P->mem->at;
	P->mem->at += size;

	return ptr;
}

static bool is_on_page(const Dit_AllocPage *page, const void *ptr)
{
	return (const char *)(ptr) >= page->raw &&
	       (const char *)(ptr) < page->raw + page->at;
}

static Dit_AllocPage *get_ptr_page(Dit_State *P, void *ptr)
{
	for (Dit_AllocPage *page = P->mem; page; page = page->prev) {
		if (is_on_page(page, ptr)) {
			return page;
		}
	}
	return NULL;
}

void *dit_alloc_resize(Dit_State *P, void *ptr, size_t from, size_t to)
{
	if (from == to) {
		return ptr;
	}

	Dit_AllocPage *page = get_ptr_page(P, ptr);
	if (page) {
		size_t offset = (const char *)(ptr)-page->raw;
		if (page->at == offset + from) {
			if (page->cap >= offset + to) {
				page->at = offset + to;
				return ptr;
			}
		}
	}

	if (from > to) {
		return ptr;
	}

	void *dest = dit_alloc(P, to);
	if (!dest) {
		return NULL;
	}
	memcpy(dest, ptr, from);

	return dest;
}
