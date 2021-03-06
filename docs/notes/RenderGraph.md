[https://github.com/acdemiralp/fg]
[https://stackoverflow.com/questions/47226029/vulkan-compute-shader-reading-only-parts-of-the-uniform-buffer]
[http://www.asawicki.info/news.php5?x=all&page=2]
[https://github.com/KhronosGroup/SPIRV-Cross]

// per-resource barriers should usually be used for queue ownership transfers and image layout transitions

/* These Map To Vulkan's Subpasses */

RenderGraph {
	void compile()
	{
		// Update ref counts

		// Cull Unused Resources and passes.

		Stack<IResource<T>*> unused_res = [...all rez with ref count of 0...];

		while (!unused_res.empty())
		{
			IResource<T>* res = unused_res.pop();
			IPass<T>* parent = res->parent;

			--parent->ref_count;

			// Can also add a check to see if this is actually immune from being culled.
			if (parent->ref_count == 0)
			{
				foreach (read_res : parent->reads)
				{
					--read_res->ref_count;

					if (read_res->ref_count == 0 && read_res->isTransient())
					{
						unused_res.push(read_res);
					}
				}
			}

			foreach (writer_res : res->writers)
			{
				--writer_res->ref_count;

				// Can also add a check to see if this is actually immune from being culled.
				if (writer_res->ref_count == 0)
				{
					// Repeated code as above.
					foreach (read_res : writer_res->reads)
					{
						--read_res->ref_count;

						if (read_res->ref_count == 0 && read_res->isTransient())
						{
							unused_res.push(read_res);
						}
					}
				}
			}
		}

		// Compilation of Passes

		IResource<T>* creates = {};
		IResource<T>* destroy = {};
		Int current_index = 0;

		foreach (render_task : this->render_tasks)
		{
			if (render_task->ref_count != 0)
			{
				creates.clear();
				destroy.clear();

				foreach (res : render_task->created_resources)
				{
					creates.push(res);

					if (res.readers.empty() && res.writers.empty())
					{
						destroy.push(res);
					}
				}

				foreach(res : (render_task->reads + render_task->writes))
				{
					if (res.isTransient())
					{
						Bool is_valid = false;
						Int  last_index = Max(
							findIndexOf(this->render_tasks, res.readers.back(), &is_valid),
							findIndexOf(this->render_tasks, res.writers.back(), &is_valid)
						);

						if (is_valid && last_index == current_index)
						{
							destroy.push(res);
						}
					}
				}

				this->actions.push(render_task, creates, destroy);
			}

			++current_index;
		}
	}

	void exec(CommandBuffer& cmds)
	{
		foreach (pass : this->actions)
		{
			foreach (res : pass.creates) { res.create(); }
			pass.execute(this->builder, cmds, pass.data);
			foreach (res : pass.destroy) { res.destroy(); }
		}
	}
};

What in Vulkan Needs Barriers:

Y - Yes we need a barrier.
N - No barrier is needed.

Color / Depth Attachments:
	- Any yes here requires a layout transition as well.
	[Y] Read  -> Write
	[Y] Write -> Read
	[Y] Write -> Write
	[N] Read  -> Read

// All the conditions to make a barrier.
  [ 0] Buffer Read -> Buffer Write.
  [ 1] Subpass Reading of the same pixel that is being written to earlier.
  [ 2] A renderpass needing the framebuffer from an earlier renderpass.
  [ 3] Buffer Write -> Buffer Read.
  [ 4] Image Write -> Image Read.
  [ 5] Image Read -> Image Write.
  [ 6] Image Read -> Image Read (Different Layout)
  [ 7] Buffer Update Operation on one Queue is needed to be synced on another queue.
  [ 8] Image Update Operation on one Queue is needed to be synced on another queue.
  [ 9] Reuse of an Image / Framebuffer in another Renderpass.
  [10] One Queue needs to Wait for Another Queue to be done.
  [11] Buffer Write -> Buffer Write.

[ 0]
  From this reader is there a writer later in this subpass.
  From this reader is there a writer later on after this renderpass.
[ 1]
  Start at each color out and find the next color in after this point.
[ 2]
  Start at this read and go back until a write is found.
[ 3] / [11]
  Start at a buffer write and check for any reads or writes.
[ 4]
  Start at image write and find the next write or read.
[ 5]
  Start at this image read and find the next write.
[ 6]
  If a change in layout is detected emit an image barrier.
[ 7]
[ 8]
[ 9]
  Start at any writes this RenderPass does and fine the next
  read or write.
[10]

[https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkAccessFlagBits.html]

Memory dependencies force data out of caches, thus ensuring that when the second write takes place, there isn't a write sitting around in a cache somewhere.
(Compute Requires: VK_BUFFER_USAGE_STORAGE_BUFFER_BIT )

// Storage image to storage image dependencies are always in GENERAL layout; no need for a layout transition
// Thus this works for images aswell.
Compute Pass Writes to Buffer but a Later Compute Pass Reads it (RAW):
  VkMemoryBarrier
    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT
    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
  vkCmdPipelineBarrier(
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
  );

Compute Pass Reads Buffer but a later pass writes to it (WAR):
  Just a simple Execution Barrier:
  vkCmdPipelineBarrier(
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
  );

Compute does some work and then later the Graphics Need it:
  VkMemoryBarrier
    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT
    .dstAccessMask = VK_ACCESS_INDEX_READ_BIT
  vkCmdPipelineBarrier(
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
  );

Compute does some work but then later Graphics needs it as well as a nother compute:
  To be more efficient just add more flags to both:
  dstAccessMask and dstStageMask

Compute works on a storage image and gfx wants to sample from it:
  VkImageMemoryBarrier
    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT
    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
    .oldLayout     = VK_IMAGE_LAYOUT_GENERAL
    .newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  vkCmdPipelineBarrier(
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
  );

Compute works on a texel buffer and gfx wants to use it (as uniform and draw indirect):
  VkMemoryBarrier
    .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT
    .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT
  vkCmdPipelineBarrier(
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
  );

Graphcis draw to image then Compute wants to read it (color vs depth):
  VkImageMemoryBarrier
    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT / VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
    .oldLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL / VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    .newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
  vkCmdPipelineBarrier(
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT / (VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT),
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
  );

Graphcics -> Graphics with subpass deps shown below:

Renderpass Has an Output (Framebuffer) that is read by a later Renderpass:
  External Subpass Dependency.
  After:
  0
  VK_SUBPASS_EXTERNAL
  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
  VK_ACCESS_SHADER_READ_BIT
  0
  or
  VkImageMemoryBarrier in the case of DepthAttachment.

Subpass has an Output (Framebuffer) read by a later Subpass
  Internal Subpass Dependency.
  Layout changes are automatic between subpasses.
  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
  VK_ACCESS_INPUT_ATTACHMENT_READ_BIT

vkCmdUpdateBuffer followed by a buffer read / write:
  Must be outside of a renderpass instance.
  This is treated as a transfer Operation.
  Less than "65536" woth of data.
  dstOffset and dataSize must be a multiple of 4.
  Pipeline barrier.

vkCmdCopyBuffer
  Must be outside of a renderpass instance.
  This is treated as a transfer Operation.
  Pipeline barrier.

vkCmdClearColorImage / vkCmdClearDepthStencilImage: can only be used outside of a RenderPass.
vkCmdClearAttachments: is for inside.
  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT or
  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT

Vulkan spec states:
  Write-after-read hazards can be solved with just an execution dependency, but read-after-write and write-after-write hazards need appropriate memory dependencies to be included between them.

Sampled Image:          A descriptor type that represents an image view, and supports filtered (sampled) and unfiltered read-only acccess in a shader.
Sampler:                An object that contains state that controls how sampled image data is sampled (or filtered) when accessed in a shader. Also a descriptor type describing the object. Represented by a VkSampler object.
Combined Image Sampler: A descriptor type that includes both a sampled image and a sampler.
Storage Image:          A descriptor type that represents an image view, and supports unfiltered loads, stores, and atomics in a shader.
Input Attachment:       A descriptor type that represents an image view, and supports unfiltered read-only access in a shader, only at the fragment’s location in the view.
Uniform Buffer:         A descriptor type that represents a buffer, and supports read-only access in a shader.
Dynamic Uniform Buffer: A uniform buffer whose offset is specified each time the uniform buffer is bound to a command buffer via a descriptor set.
Uniform Texel Buffer:   A descriptor type that represents a buffer view, and supports unfiltered, formatted, read-only access in a shader.
Storage Buffer:         A descriptor type that represents a buffer, and supports reads, writes, and atomics in a shader.
Dynamic Storage Buffer: A storage buffer whose offset is specified each time the storage buffer is bound to a command buffer via a descriptor set.
Storage Texel Buffer:   A descriptor type that represents a buffer view, and supports unfiltered, formatted reads, writes, and atomics in a shader.


Full screen Quad is better than a Blit Operation:
It's possible that the swapchain image does not support being a blit destination.
Only being a COLOR_ATTACHMENT is required to be supported.

A Framebuffer is just a list of ImageViews + Renderpass.
Easy to generate on the fly.

Can Rendepass be generated too?
	> Color Inputs
	> Color Outputs
	> Depth Input
	> Depth Output
	> Load Ops
	> Store Ops
	> Stencil Load Ops
	> Stencil Store Ops
	> Subpass dependencies
Should be easy ASSUMING the FrameGraph has enough context.

Pipeline can be generated to.

Buffers can just use a chain of free blocks. (This is simple but should be effective)
Maybe evict after X frames of unuse.

Queue Info:

If doing a lot of cross queue sync you are using them wrong.

Host -> Device:   Use a Transfer Queue.
Device -> Device: Your normal Graphics Queue is fine.

IF the Compute feeds gfx then use the same queue.

Only use more queues if you submit completely independent work.

Present and Graphics Queue could actually be diff according to the spec.

// Compute needs a layout of VK_IMAGE_LAYOUT_GENERAL

So I was having issues understanding subpass deps since I did not know aquiring and image 
basically counts as a command.

When you write to the Swapchain's image you need to wait for the image to be available
which I have been using ".waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}"
so the renderpass that uses this 
