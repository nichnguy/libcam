/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2018, Google Inc.
 *
 * pipeline_handler.cpp - Pipeline handler infrastructure
 */

#include <libcamera/buffer.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>

#include "log.h"
#include "media_device.h"
#include "pipeline_handler.h"
#include "utils.h"

/**
 * \file pipeline_handler.h
 * \brief Create pipelines and cameras from a set of media devices
 *
 * Each pipeline supported by libcamera needs to be backed by a pipeline
 * handler implementation that operate on a set of media devices. The pipeline
 * handler is responsible for matching the media devices it requires with the
 * devices present in the system, and once all those devices can be acquired,
 * create corresponding Camera instances.
 *
 * Every subclass of PipelineHandler shall be registered with libcamera using
 * the REGISTER_PIPELINE_HANDLER() macro.
 */

namespace libcamera {

LOG_DEFINE_CATEGORY(Pipeline)

/**
 * \class CameraData
 * \brief Base class for platform-specific data associated with a camera
 *
 * The CameraData base abstract class represents platform specific-data
 * a pipeline handler might want to associate with a Camera to access them
 * at a later time.
 *
 * Pipeline handlers are expected to extend this base class with platform
 * specific implementation, associate instances of the derived classes
 * using the setCameraData() method, and access them at a later time
 * with cameraData().
 */

/**
 * \fn CameraData::CameraData(PipelineHandler *pipe)
 * \brief Construct a CameraData instance for the given pipeline handler
 * \param[in] pipe The pipeline handler
 *
 * The reference to the pipeline handler is stored internally, the caller shall
 * guarantee that the pointer remains valid as long as the CameraData instance
 * exists.
 */

/**
 * \var CameraData::camera_
 * \brief The camera related to this CameraData instance
 *
 * The camera_ pointer provides access to the Camera object that this instance
 * is related to. It is set when the Camera is registered with
 * PipelineHandler::registerCamera() and remains valid until the CameraData
 * instance is destroyed.
 */

/**
 * \var CameraData::pipe_
 * \brief The pipeline handler related to this CameraData instance
 *
 * The pipe_ pointer provides access to the PipelineHandler object that this
 * instance is related to. It is set when the CameraData instance is created
 * and remains valid until the instance is destroyed.
 */

/**
 * \var CameraData::queuedRequests_
 * \brief The list of queued and not yet completed request
 *
 * The list of queued request is used to track requests queued in order to
 * ensure completion of all requests when the pipeline handler is stopped.
 *
 * \sa PipelineHandler::queueRequest(), PipelineHandler::stop(),
 * PipelineHandler::completeRequest()
 */

/**
 * \class PipelineHandler
 * \brief Create and manage cameras based on a set of media devices
 *
 * The PipelineHandler matches the media devices provided by a DeviceEnumerator
 * with the pipelines it supports and creates corresponding Camera devices.
 *
 * Pipeline handler instances are reference-counted through std::shared_ptr<>.
 * They implement std::enable_shared_from_this<> in order to create new
 * std::shared_ptr<> in code paths originating from member functions of the
 * PipelineHandler class where only the 'this' pointer is available.
 */

/**
 * \brief Construct a PipelineHandler instance
 * \param[in] manager The camera manager
 *
 * In order to honour the std::enable_shared_from_this<> contract,
 * PipelineHandler instances shall never be constructed manually, but always
 * through the PipelineHandlerFactory::create() method implemented by the
 * respective factories.
 */
PipelineHandler::PipelineHandler(CameraManager *manager)
	: manager_(manager)
{
}

PipelineHandler::~PipelineHandler()
{
};

/**
 * \fn PipelineHandler::match(DeviceEnumerator *enumerator)
 * \brief Match media devices and create camera instances
 * \param enumerator The enumerator providing all media devices found in the
 * system
 *
 * This function is the main entry point of the pipeline handler. It is called
 * by the camera manager with the \a enumerator passed as an argument. It shall
 * acquire from the \a enumerator all the media devices it needs for a single
 * pipeline, create one or multiple Camera instances and register them with the
 * camera manager.
 *
 * If all media devices needed by the pipeline handler are found, they must all
 * be acquired by a call to MediaDevice::acquire(). This function shall then
 * create the corresponding Camera instances, store them internally, and return
 * true. Otherwise it shall not acquire any media device (or shall release all
 * the media devices is has acquired by calling MediaDevice::release()) and
 * return false.
 *
 * If multiple instances of a pipeline are available in the system, the
 * PipelineHandler class will be instanciated once per instance, and its match()
 * function called for every instance. Each call shall acquire media devices for
 * one pipeline instance, until all compatible media devices are exhausted.
 *
 * If this function returns true, a new instance of the pipeline handler will
 * be created and its match() function called.
 *
 * \return true if media devices have been acquired and camera instances
 * created, or false otherwise
 */

/**
 * \fn PipelineHandler::streamConfiguration()
 * \brief Retrieve a group of stream configurations for a specified camera
 * \param[in] camera The camera to fetch default configuration from
 * \param[in] usages A list of stream usages
 *
 * Retrieve the species camera's default configuration for a specified group of
 * use-cases. The caller shall populate the \a usages array with the use-cases it
 * wishes to fetch the default configuration for. The map of streams and
 * configurations returned can then be examined by the caller to learn about
 * the default parameters for the specified streams.
 *
 * The intended companion to this is \a configureStreams() which can be used to
 * change the group of streams parameters.
 *
 * \return A valid CameraConfiguration if the requested usages can be satisfied,
 * or a invalid configuration otherwise
 */

/**
 * \fn PipelineHandler::configureStreams()
 * \brief Configure a group of streams for capture
 * \param[in] camera The camera to configure
 * \param[in] config The camera configurations to setup
 *
 * Configure the specified group of streams for \a camera according to the
 * configuration specified in \a configs. The intended caller of this interface
 * is the Camera class which will receive configuration to apply from the
 * application.
 *
 * Each pipeline handler implementation is responsible for validating
 * that the configuration requested in \a config can be achieved
 * exactly. Any difference in pixel format, frame size or any other
 * parameter shall result in the -EINVAL error being returned, and no
 * change in configuration being applied to the pipeline. If
 * configuration of a subset of the streams can't be satisfied, the
 * whole configuration is considered invalid.
 *
 * \return 0 on success or a negative error code otherwise
 */

/**
 * \fn PipelineHandler::allocateBuffers()
 * \brief Allocate buffers for a stream
 * \param[in] camera The camera the \a stream belongs to
 * \param[in] streams The set of streams to allocate buffers for
 *
 * This method allocates buffers internally in the pipeline handler for each
 * stream in the \a streams buffer set, and associates them with the stream's
 * buffer pool.
 *
 * The intended caller of this method is the Camera class.
 *
 * \return 0 on success or a negative error code otherwise
 */

/**
 * \fn PipelineHandler::freeBuffers()
 * \brief Free all buffers associated with a stream
 * \param[in] camera The camera the \a stream belongs to
 * \param[in] streams The set of streams to free buffers from
 *
 * After a capture session has been stopped all buffers associated with each
 * stream shall be freed.
 *
 * The intended caller of this method is the Camera class.
 *
 * \return 0 on success or a negative error code otherwise
 */

/**
 * \fn PipelineHandler::start()
 * \brief Start capturing from a group of streams
 * \param[in] camera The camera to start
 *
 * Start the group of streams that have been configured for capture by
 * \a configureStreams(). The intended caller of this method is the Camera
 * class which will in turn be called from the application to indicate that it
 * has configured the streams and is ready to capture.
 *
 * \return 0 on success or a negative error code otherwise
 */

/**
 * \fn PipelineHandler::stop()
 * \brief Stop capturing from all running streams
 * \param[in] camera The camera to stop
 *
 * This method stops capturing and processing requests immediately. All pending
 * requests are cancelled and complete immediately in an error state.
 *
 * Pipeline handlers shall override this method to stop the pipeline, ensure
 * that all pending request completion signaled through completeRequest() have
 * returned, and call the base implementation of the stop() method as the last
 * step of their implementation. The base implementation cancels all requests
 * queued but not yet complete.
 */
void PipelineHandler::stop(Camera *camera)
{
	CameraData *data = cameraData(camera);

	while (!data->queuedRequests_.empty()) {
		Request *request = data->queuedRequests_.front();
		data->queuedRequests_.pop_front();

		while (!request->pending_.empty()) {
			Buffer *buffer = *request->pending_.begin();
			buffer->cancel();
			completeBuffer(camera, request, buffer);
		}

		request->complete(Request::RequestCancelled);
		camera->requestComplete(request);
	}
}

/**
 * \fn PipelineHandler::queueRequest()
 * \brief Queue a request to the camera
 * \param[in] camera The camera to queue the request to
 * \param[in] request The request to queue
 *
 * This method queues a capture request to the pipeline handler for processing.
 * The request contains a set of buffers associated with streams and a set of
 * parameters. The pipeline handler shall program the device to ensure that the
 * parameters will be applied to the frames captured in the buffers provided in
 * the request.
 *
 * Pipeline handlers shall override this method. The base implementation in the
 * PipelineHandler class keeps track of queued requests in order to ensure
 * completion of all requests when the pipeline handler is stopped with stop().
 * Requests completion shall be signaled by the pipeline handler using the
 * completeRequest() method.
 *
 * \return 0 on success or a negative error code otherwise
 */
int PipelineHandler::queueRequest(Camera *camera, Request *request)
{
	CameraData *data = cameraData(camera);
	data->queuedRequests_.push_back(request);

	return 0;
}

/**
 * \brief Complete a buffer for a request
 * \param[in] camera The camera the request belongs to
 * \param[in] request The request the buffer belongs to
 * \param[in] buffer The buffer that has completed
 *
 * This method shall be called by pipeline handlers to signal completion of the
 * \a buffer part of the \a request. It notifies applications of buffer
 * completion and updates the request's internal buffer tracking. The request
 * is not completed automatically when the last buffer completes, pipeline
 * handlers shall complete requests explicitly with completeRequest().
 *
 * \return True if all buffers contained in the request have completed, false
 * otherwise
 */
bool PipelineHandler::completeBuffer(Camera *camera, Request *request,
				     Buffer *buffer)
{
	camera->bufferCompleted.emit(request, buffer);
	return request->completeBuffer(buffer);
}

/**
 * \brief Signal request completion
 * \param[in] camera The camera that the request belongs to
 * \param[in] request The request that has completed
 *
 * The pipeline handler shall call this method to notify the \a camera that the
 * request request has complete. The request is deleted and shall not be
 * accessed once this method returns.
 *
 * The pipeline handler shall ensure that requests complete in the same order
 * they are submitted.
 */
void PipelineHandler::completeRequest(Camera *camera, Request *request)
{
	CameraData *data = cameraData(camera);
	ASSERT(request == data->queuedRequests_.front());
	data->queuedRequests_.pop_front();

	request->complete(Request::RequestComplete);
	camera->requestComplete(request);
}

/**
 * \brief Register a camera to the camera manager and pipeline handler
 * \param[in] camera The camera to be added
 * \param[in] data Pipeline-specific data for the camera
 *
 * This method is called by pipeline handlers to register the cameras they
 * handle with the camera manager. It associates the pipeline-specific \a data
 * with the camera, for later retrieval with cameraData(). Ownership of \a data
 * is transferred to the PipelineHandler.
 */
void PipelineHandler::registerCamera(std::shared_ptr<Camera> camera,
				     std::unique_ptr<CameraData> data)
{
	data->camera_ = camera.get();
	cameraData_[camera.get()] = std::move(data);
	cameras_.push_back(camera);
	manager_->addCamera(std::move(camera));
}

/**
 * \brief Enable hotplug handling for a media device
 * \param[in] media The media device
 *
 * This function enables hotplug handling, and especially hot-unplug handling,
 * of the \a media device. It shall be called by pipeline handlers for all the
 * media devices that can be disconnected.
 *
 * When a media device passed to this function is later unplugged, the pipeline
 * handler gets notified and automatically disconnects all the cameras it has
 * registered without requiring any manual intervention.
 */
void PipelineHandler::hotplugMediaDevice(MediaDevice *media)
{
	media->disconnected.connect(this, &PipelineHandler::mediaDeviceDisconnected);
}

/**
 * \brief Slot for the MediaDevice disconnected signal
 */
void PipelineHandler::mediaDeviceDisconnected(MediaDevice *media)
{
	media->disconnected.disconnect(this);

	if (cameras_.empty())
		return;

	disconnect();
}

/**
 * \brief Device disconnection handler
 *
 * This virtual function is called to notify the pipeline handler that the
 * device it handles has been disconnected. It notifies all cameras created by
 * the pipeline handler that they have been disconnected, and unregisters them
 * from the camera manager.
 *
 * The function can be overloaded by pipeline handlers to perform custom
 * operations at disconnection time. Any overloaded version shall call the
 * PipelineHandler::disconnect() base function for proper hot-unplug operation.
 */
void PipelineHandler::disconnect()
{
	for (std::weak_ptr<Camera> ptr : cameras_) {
		std::shared_ptr<Camera> camera = ptr.lock();
		if (!camera)
			continue;

		camera->disconnect();
		manager_->removeCamera(camera.get());
	}

	cameras_.clear();
}

/**
 * \brief Retrieve the pipeline-specific data associated with a Camera
 * \param camera The camera whose data to retrieve
 *
 * \return A pointer to the pipeline-specific data passed to registerCamera().
 * The returned pointer is a borrowed reference and is guaranteed to remain
 * valid until the pipeline handler is destroyed. It shall not be deleted
 * manually by the caller.
 */
CameraData *PipelineHandler::cameraData(const Camera *camera)
{
	ASSERT(cameraData_.count(camera));
	return cameraData_[camera].get();
}

/**
 * \var PipelineHandler::manager_
 * \brief The Camera manager associated with the pipeline handler
 *
 * The camera manager pointer is stored in the pipeline handler for the
 * convenience of pipeline handler implementations. It remains valid and
 * constant for the whole lifetime of the pipeline handler.
 */

/**
 * \class PipelineHandlerFactory
 * \brief Registration of PipelineHandler classes and creation of instances
 *
 * To facilitate discovery and instantiation of PipelineHandler classes, the
 * PipelineHandlerFactory class maintains a registry of pipeline handler
 * classes. Each PipelineHandler subclass shall register itself using the
 * REGISTER_PIPELINE_HANDLER() macro, which will create a corresponding
 * instance of a PipelineHandlerFactory subclass and register it with the
 * static list of factories.
 */

/**
 * \brief Construct a pipeline handler factory
 * \param[in] name Name of the pipeline handler class
 *
 * Creating an instance of the factory registers is with the global list of
 * factories, accessible through the factories() function.
 *
 * The factory \a name is used for debug purpose and shall be unique.
 */
PipelineHandlerFactory::PipelineHandlerFactory(const char *name)
	: name_(name)
{
	registerType(this);
}

/**
 * \fn PipelineHandlerFactory::create()
 * \brief Create an instance of the PipelineHandler corresponding to the factory
 * \param[in] manager The camera manager
 *
 * This virtual function is implemented by the REGISTER_PIPELINE_HANDLER()
 * macro. It creates a pipeline handler instance associated with the camera
 * \a manager.
 *
 * \return a pointer to a newly constructed instance of the PipelineHandler
 * subclass corresponding to the factory
 */

/**
 * \fn PipelineHandlerFactory::name()
 * \brief Retrieve the factory name
 * \return The factory name
 */

/**
 * \brief Add a pipeline handler class to the registry
 * \param[in] factory Factory to use to construct the pipeline handler
 *
 * The caller is responsible to guarantee the uniqueness of the pipeline handler
 * name.
 */
void PipelineHandlerFactory::registerType(PipelineHandlerFactory *factory)
{
	std::vector<PipelineHandlerFactory *> &factories = PipelineHandlerFactory::factories();

	factories.push_back(factory);

	LOG(Pipeline, Debug)
		<< "Registered pipeline handler \"" << factory->name() << "\"";
}

/**
 * \brief Retrieve the list of all pipeline handler factories
 *
 * The static factories map is defined inside the function to ensures it gets
 * initialized on first use, without any dependency on link order.
 *
 * \return the list of pipeline handler factories
 */
std::vector<PipelineHandlerFactory *> &PipelineHandlerFactory::factories()
{
	static std::vector<PipelineHandlerFactory *> factories;
	return factories;
}

/**
 * \def REGISTER_PIPELINE_HANDLER
 * \brief Register a pipeline handler with the pipeline handler factory
 * \param[in] handler Class name of PipelineHandler derived class to register
 *
 * Register a PipelineHandler subclass with the factory and make it available to
 * try and match devices.
 */

} /* namespace libcamera */
