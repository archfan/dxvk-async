#pragma once

#include "dxvk_format.h"
#include "dxvk_memory.h"
#include "dxvk_resource.h"
#include "dxvk_util.h"

namespace dxvk {
  
  /**
   * \brief Image create info
   * 
   * The properties of an image that are
   * passed to \ref DxvkDevice::createImage
   */
  struct DxvkImageCreateInfo {
    /// Image dimension
    VkImageType type;
    
    /// Pixel format
    VkFormat format;
    
    /// Flags
    VkImageCreateFlags flags;
    
    /// Sample count for MSAA
    VkSampleCountFlagBits sampleCount;
    
    /// Image size, in texels
    VkExtent3D extent;
    
    /// Number of image array layers
    uint32_t numLayers;
    
    /// Number of mip levels
    uint32_t mipLevels;
    
    /// Image usage flags
    VkImageUsageFlags usage;
    
    /// Pipeline stages that can access
    /// the contents of the image
    VkPipelineStageFlags stages;
    
    /// Allowed access pattern
    VkAccessFlags access;
    
    /// Image tiling mode
    VkImageTiling tiling;
    
    /// Common image layout
    VkImageLayout layout;

    // Image view formats that can
    // be used with this image
    uint32_t        viewFormatCount = 0;
    const VkFormat* viewFormats     = nullptr;
  };
  
  
  /**
   * \brief Image create info
   * 
   * The properties of an image view that are
   * passed to \ref DxvkDevice::createImageView
   */
  struct DxvkImageViewCreateInfo {
    /// Image view dimension
    VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;
    
    /// Pixel format
    VkFormat format = VK_FORMAT_UNDEFINED;

    /// Image view usage flags
    VkImageUsageFlags usage = 0;
    
    /// Subresources to use in the view
    VkImageAspectFlags aspect = 0;
    
    uint32_t minLevel  = 0;
    uint32_t numLevels = 0;
    uint32_t minLayer  = 0;
    uint32_t numLayers = 0;
    
    /// Component mapping. Defaults to identity.
    VkComponentMapping swizzle = {
      VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY,
      VK_COMPONENT_SWIZZLE_IDENTITY,
    };
  };
  
  
  /**
   * \brief DXVK image
   * 
   * An image resource consisting of various subresources.
   * Can be accessed by the host if allocated on a suitable
   * memory type and if created with the linear tiling option.
   */
  class DxvkImage : public DxvkResource {
    
  public:
    
    DxvkImage(
      const Rc<vk::DeviceFn>&     vkd,
      const DxvkImageCreateInfo&  createInfo,
            DxvkMemoryAllocator&  memAlloc,
            VkMemoryPropertyFlags memFlags);
    
    /**
     * \brief Creates image object from existing image
     * 
     * This can be used to create an image object for
     * an implementation-managed image. Make sure to
     * provide the correct image properties, since
     * otherwise some image operations may fail.
     */
    DxvkImage(
      const Rc<vk::DeviceFn>&     vkd,
      const DxvkImageCreateInfo&  info,
            VkImage               image);
    
    /**
     * \brief Destroys image
     * 
     * If this is an implementation-managed image,
     * this will not destroy the Vulkan image.
     */
    ~DxvkImage();
    
    /**
     * \brief Image handle
     * 
     * Internal use only.
     * \returns Image handle
     */
    VkImage handle() const {
      return m_image;
    }
    
    /**
     * \brief Image properties
     * 
     * The image create info structure.
     * \returns Image properties
     */
    const DxvkImageCreateInfo& info() const {
      return m_info;
    }
    
    /**
     * \brief Memory type flags
     * 
     * Use this to determine whether a
     * buffer is mapped to host memory.
     * \returns Vulkan memory flags
     */
    VkMemoryPropertyFlags memFlags() const {
      return m_memFlags;
    }
    
    /**
     * \brief Map pointer
     * 
     * If the image has been created on a host-visible
     * memory type, its memory is mapped and can be
     * accessed by the host.
     * \param [in] offset Byte offset into mapped region
     * \returns Pointer to mapped memory region
     */
    void* mapPtr(VkDeviceSize offset) const {
      return m_memory.mapPtr(offset);
    }
    
    /**
     * \brief Image format info
     * \returns Image format info
     */
    const DxvkFormatInfo* formatInfo() const {
      return imageFormatInfo(m_info.format);
    }
    
    /**
     * \brief Size of a mipmap level
     * 
     * \param [in] level Mip level
     * \returns Size of that level
     */
    VkExtent3D mipLevelExtent(uint32_t level) const {
      VkExtent3D size = m_info.extent;
      size.width  = std::max(1u, size.width  >> level);
      size.height = std::max(1u, size.height >> level);
      size.depth  = std::max(1u, size.depth  >> level);
      return size;
    }
    
    /**
     * \brief Queries memory layout of a subresource
     * 
     * Can be used to retrieve the exact pointer to a
     * subresource of a mapped image with linear tiling.
     * \param [in] subresource The image subresource
     * \returns Memory layout of that subresource
     */
    VkSubresourceLayout querySubresourceLayout(
      const VkImageSubresource& subresource) const {
      VkSubresourceLayout result;
      m_vkd->vkGetImageSubresourceLayout(
        m_vkd->device(), m_image,
        &subresource, &result);
      return result;
    }
    
    /**
     * \brief Picks a compatible layout
     * 
     * Under some circumstances, we have to return
     * a different layout than the one requested.
     * \param [in] layout The image layout
     * \returns A compatible image layout
     */
    VkImageLayout pickLayout(VkImageLayout layout) const {
      return m_info.layout == VK_IMAGE_LAYOUT_GENERAL
        ? VK_IMAGE_LAYOUT_GENERAL : layout;
    }

    /**
     * \brief Checks whether a subresource is entirely covered
     * 
     * This can be used to determine whether an image can or
     * should be initialized with \c VK_IMAGE_LAYOUT_UNDEFINED.
     * \param [in] subresource The image subresource
     * \param [in] extent Image extent to check
     */
    bool isFullSubresource(
      const VkImageSubresourceLayers& subresource,
            VkExtent3D                extent) const {
      return subresource.aspectMask == this->formatInfo()->aspectMask
          && extent == this->mipLevelExtent(subresource.mipLevel);
    }
    
  private:
    
    Rc<vk::DeviceFn>      m_vkd;
    DxvkImageCreateInfo   m_info;
    VkMemoryPropertyFlags m_memFlags;
    DxvkMemory            m_memory;
    VkImage               m_image = VK_NULL_HANDLE;

    std::vector<VkFormat> m_viewFormats;
    
  };
  
  
  /**
   * \brief DXVK image view
   */
  class DxvkImageView : public DxvkResource {
    constexpr static uint32_t ViewCount = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY + 1;
  public:
    
    DxvkImageView(
      const Rc<vk::DeviceFn>&         vkd,
      const Rc<DxvkImage>&            image,
      const DxvkImageViewCreateInfo&  info);
    
    ~DxvkImageView();
    
    /**
     * \brief Image view handle for the default type
     * 
     * The default view type is guaranteed to be
     * supported by the image view, and should be
     * preferred over picking a different type.
     * \returns Image view handle
     */
    VkImageView handle() const {
      return handle(m_info.type);
    }
    
    /**
     * \brief Image view handle for a given view type
     * 
     * If the view does not support the requested image
     * view type, \c VK_NULL_HANDLE will be returned.
     * \param [in] viewType The requested view type
     * \returns The image view handle
     */
    VkImageView handle(VkImageViewType viewType) const {
      return m_views[viewType];
    }
    
    /**
     * \brief Image view type
     * 
     * Convenience method to query the view type
     * in order to check for resource compatibility.
     * \returns Image view type
     */
    VkImageViewType type() const {
      return m_info.type;
    }
    
    /**
     * \brief Image view properties
     * \returns Image view properties
     */
    const DxvkImageViewCreateInfo& info() const {
      return m_info;
    }
    
    /**
     * \brief Image handle
     * \returns Image handle
     */
    VkImage imageHandle() const {
      return m_image->handle();
    }
    
    /**
     * \brief Image properties
     * \returns Image properties
     */
    const DxvkImageCreateInfo& imageInfo() const {
      return m_image->info();
    }
    
    /**
     * \brief Image format info
     * \returns Image format info
     */
    const DxvkFormatInfo* formatInfo() const {
      return m_image->formatInfo();
    }
    
    /**
     * \brief Image object
     * \returns Image object
     */
    const Rc<DxvkImage>& image() const {
      return m_image;
    }
    
    /**
     * \brief Mip level size
     * 
     * Computes the mip level size relative to
     * the first mip level that the view includes.
     * \param [in] level Mip level
     * \returns Size of that level
     */
    VkExtent3D mipLevelExtent(uint32_t level) const {
      return m_image->mipLevelExtent(level + m_info.minLevel);
    }
    
    /**
     * \brief Subresource range
     * \returns Subresource range
     */
    VkImageSubresourceRange subresources() const {
      VkImageSubresourceRange result;
      result.aspectMask     = m_info.aspect;
      result.baseMipLevel   = m_info.minLevel;
      result.levelCount     = m_info.numLevels;
      result.baseArrayLayer = m_info.minLayer;
      result.layerCount     = m_info.numLayers;
      return result;
    }
    
    /**
     * \brief Picks an image layout
     * \see DxvkImage::pickLayout
     */
    VkImageLayout pickLayout(VkImageLayout layout) const {
      return m_image->pickLayout(layout);
    }

    /**
     * \brief Sets render target usage frame number
     * 
     * The image view will track internally when
     * it was last used as a render target. This
     * info is used for async shader compilation.
     * \param [in] frameId Frame number
     */
    void setRtBindingFrameId(uint32_t frameId) {
      if (frameId != m_rtBindingFrameId) {
        if (frameId == m_rtBindingFrameId + 1)
          m_rtBindingFrameCount += 1;
        else
          m_rtBindingFrameCount = 0;
        
        m_rtBindingFrameId = frameId;
      }
    }

    /**
     * \brief Checks for async pipeline compatibility
     * 
     * Asynchronous pipeline compilation may be enabled if the
     * render target has been drawn to in the previous frames.
     * \param [in] frameId Current frame ID
     * \returns \c true if async compilation is supported
     */
    bool getRtBindingAsyncCompilationCompat() const {
      return m_rtBindingFrameCount >= 5;
    }

  private:
    
    Rc<vk::DeviceFn>  m_vkd;
    Rc<DxvkImage>     m_image;
    
    DxvkImageViewCreateInfo m_info;
    VkImageView             m_views[ViewCount];

    uint32_t m_rtBindingFrameId    = 0;
    uint32_t m_rtBindingFrameCount = 0;

    void createView(VkImageViewType type, uint32_t numLayers);
    
  };
  
}