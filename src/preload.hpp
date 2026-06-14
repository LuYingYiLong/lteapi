#ifndef PRELOAD_HPP
#define PRELOAD_HPP

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>

namespace godot {
	// 单例资源缓存管理器
	class PreloadCache {
	public:
		static PreloadCache& get_singleton() {
			static PreloadCache instance;
			return instance;
		}

		// 禁止拷贝
		PreloadCache(const PreloadCache&) = delete;
		PreloadCache& operator=(const PreloadCache&) = delete;

		// 获取或加载资源
		template<typename T>
		Ref<T> get_or_load(const String& path) {
			std::lock_guard<std::mutex> lock(cache_mutex_);

			std::string key = path.utf8().get_data();
			auto it = cache_.find(key);

			if (it != cache_.end()) {
				// 已缓存，尝试转换类型
				Ref<Resource> res = it->second;
				if (res.is_valid()) {
					Ref<T> typed_res = res;
					if (typed_res.is_valid()) {
						return typed_res;
					}
				}
				// 类型不匹配或资源失效，重新加载
			}

			// 加载新资源
			Ref<T> resource = ResourceLoader::get_singleton()->load(path);
			if (resource.is_valid()) {
				cache_[key] = resource;
			}
			else {
				UtilityFunctions::push_error("Preload failed: " + path);
			}

			return resource;
		}

		// 预加载（不返回，仅填充缓存）
		template<typename T>
		void preload(const String& path) {
			get_or_load<T>(path);
		}

		// 清除特定资源
		void unload(const String& path) {
			std::lock_guard<std::mutex> lock(cache_mutex_);
			cache_.erase(path.utf8().get_data());
		}

		// 清空缓存
		void clear() {
			std::lock_guard<std::mutex> lock(cache_mutex_);
			cache_.clear();
		}

		// 获取缓存大小
		size_t size() const {
			return cache_.size();
		}

	private:
		PreloadCache() = default;
		~PreloadCache() = default;

		std::unordered_map<std::string, Ref<Resource>> cache_;
		mutable std::mutex cache_mutex_;
	};

	// ============================================
	// 模板封装：类似 Godot 的 preload() 函数
	// ============================================

	template<typename T = Resource>
	class Preloaded {
	public:
		Preloaded() = default;

		// 从路径构造，立即加载
		explicit Preloaded(const String& path) : path_(path) {
			load();
		}

		// 延迟加载（首次访问时加载）
		explicit Preloaded(const String& path, bool lazy) : path_(path) {
			if (!lazy) {
				load();
			}
		}

		// 获取资源（自动加载）
		Ref<T> get() {
			if (!resource_.is_valid() && !path_.is_empty()) {
				load();
			}
			return resource_;
		}

		// 强制重新加载
		Ref<T> reload() {
			load();
			return resource_;
		}

		// 解引用操作符
		Ref<T> operator->() {
			return get();
		}

		Ref<T> operator*() {
			return get();
		}

		// 布尔转换
		explicit operator bool() const {
			return resource_.is_valid();
		}

		// 获取路径
		String get_path() const {
			return path_;
		}

		// 检查是否已加载
		bool is_loaded() const {
			return resource_.is_valid();
		}

		// 释放资源
		void unload() {
			resource_.unref();
		}

	private:
		void load() {
			if (!path_.is_empty()) {
				resource_ = PreloadCache::get_singleton().get_or_load<T>(path_);
			}
		}

		String path_;
		Ref<T> resource_;
	};

	// ============================================
	// 便捷函数（类似 Godot 的 preload()）
	// ============================================

	template<typename T = Resource>
	inline Preloaded<T> preload(const String& path) {
		return Preloaded<T>(path);
	}

	// 延迟加载版本
	template<typename T = Resource>
	inline Preloaded<T> preload_lazy(const String& path) {
		return Preloaded<T>(path, true);
	}

	// 直接获取缓存资源（不包装）
	template<typename T = Resource>
	inline Ref<T> preload_ref(const String& path) {
		return PreloadCache::get_singleton().get_or_load<T>(path);
	}

}

#endif // !PRELOAD_HPP