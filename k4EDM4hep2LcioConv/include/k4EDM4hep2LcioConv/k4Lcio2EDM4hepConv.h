#ifndef K4EDM4HEP2LCIOCONV_K4LCIO2EDM4HEPCONV_H
#define K4EDM4HEP2LCIOCONV_K4LCIO2EDM4HEPCONV_H

// EDM4hep
#include "edm4hep/CaloHitContributionCollection.h"
#include "edm4hep/CalorimeterHitCollection.h"
#include "edm4hep/ClusterCollection.h"
#include "edm4hep/EventHeaderCollection.h"
#include "edm4hep/MCParticleCollection.h"
#include "edm4hep/MCRecoCaloAssociationCollection.h"
#include "edm4hep/MCRecoCaloParticleAssociationCollection.h"
#include "edm4hep/MCRecoClusterParticleAssociationCollection.h"
#include "edm4hep/MCRecoParticleAssociationCollection.h"
#include "edm4hep/MCRecoTrackParticleAssociationCollection.h"
#include "edm4hep/MCRecoTrackerAssociationCollection.h"
#include "edm4hep/MCRecoTrackerHitPlaneAssociationCollection.h"
#include "edm4hep/ParticleIDCollection.h"
#include "edm4hep/RawCalorimeterHitCollection.h"
#include "edm4hep/RecoParticleVertexAssociationCollection.h"
#include "edm4hep/ReconstructedParticleCollection.h"
#include "edm4hep/SimCalorimeterHitCollection.h"
#include "edm4hep/SimTrackerHitCollection.h"
#include "edm4hep/RawTimeSeriesCollection.h"
#include "edm4hep/TrackCollection.h"
#include "edm4hep/TrackerHitCollection.h"
#include "edm4hep/TrackerHitPlaneCollection.h"
#include "edm4hep/VertexCollection.h"

// LCIO
#include <EVENT/CalorimeterHit.h>
#include <EVENT/Cluster.h>
#include <EVENT/LCCollection.h>
#include <EVENT/LCEvent.h>
#include <EVENT/LCRelation.h>
#include <EVENT/MCParticle.h>
#include <EVENT/ParticleID.h>
#include <EVENT/RawCalorimeterHit.h>
#include <EVENT/ReconstructedParticle.h>
#include <EVENT/SimCalorimeterHit.h>
#include <EVENT/SimTrackerHit.h>
#include <EVENT/TPCHit.h>
#include <EVENT/Track.h>
#include <EVENT/TrackState.h>
#include <EVENT/TrackerHit.h>
#include <EVENT/TrackerHitPlane.h>
#include <EVENT/Vertex.h>
#include <EVENT/LCIntVec.h>
#include <EVENT/LCFloatVec.h>
#include <UTIL/LCIterator.h>
#include <lcio.h>

#include "podio/Frame.h"
#include "podio/UserDataCollection.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <tuple>

namespace LCIO2EDM4hepConv {
  template<typename LcioT, typename EdmT>
  using TypeMapT = std::unordered_map<LcioT, EdmT>;

  /**
   * Maping holding all the original and converted objects in a 1:1 mapping in a
   * way that makes the lookup from LCIO to EDM4hep easy.
   */
  struct LcioEdmTypeMapping {
    TypeMapT<const lcio::Track*, edm4hep::MutableTrack> tracks {};
    TypeMapT<const lcio::TrackerHit*, edm4hep::MutableTrackerHit> trackerHits {};
    TypeMapT<const lcio::SimTrackerHit*, edm4hep::MutableSimTrackerHit> simTrackerHits {};
    TypeMapT<const lcio::CalorimeterHit*, edm4hep::MutableCalorimeterHit> caloHits {};
    TypeMapT<const lcio::RawCalorimeterHit*, edm4hep::MutableRawCalorimeterHit> rawCaloHits {};
    TypeMapT<const lcio::SimCalorimeterHit*, edm4hep::MutableSimCalorimeterHit> simCaloHits {};
    TypeMapT<const lcio::TPCHit*, edm4hep::MutableRawTimeSeries> tpcHits {};
    TypeMapT<const lcio::Cluster*, edm4hep::MutableCluster> clusters {};
    TypeMapT<const lcio::Vertex*, edm4hep::MutableVertex> vertices {};
    TypeMapT<const lcio::ReconstructedParticle*, edm4hep::MutableReconstructedParticle> recoParticles {};
    TypeMapT<const lcio::MCParticle*, edm4hep::MutableMCParticle> mcParticles {};
    TypeMapT<const lcio::TrackerHitPlane*, edm4hep::MutableTrackerHitPlane> trackerHitPlanes {};
    TypeMapT<const lcio::ParticleID*, edm4hep::MutableParticleID> particleIDs {};
  };

  using CollNamePair = std::tuple<std::string, std::unique_ptr<podio::CollectionBase>>;

  /*
   * Convert a LCRunHeader to EDM4hep as a frame.
   */
  podio::Frame convertRunHeader(EVENT::LCRunHeader* rheader);

  /**
   * Convert a complete LCEvent from LCIO to EDM4hep
   */
  podio::Frame convertEvent(EVENT::LCEvent* evt);

  /**
   * Convert an LCIOCollection by dispatching to the specific conversion
   * function for the corresponding type (after querying the input collection).
   * Populates the correct object mapping along the way.
   *
   * Returns a vector of names and collections (since some LCIO collections will
   * result in more than one EDM4hep collection)
   */
  std::vector<CollNamePair>
  convertCollection(const std::string& name, EVENT::LCCollection* LCCollection, LcioEdmTypeMapping& typeMapping);

  /**
   * Resolve all relations in all converted objects that are held in the map.
   * Dispatch to the correpsonding implementation for all the types that have
   * relations
   */
  void resolveRelations(LcioEdmTypeMapping& typeMapping);

  /**
   * Convert LCRelation collections into the corresponding Association collections in EDM4hep
   */
  std::vector<CollNamePair> createAssociations(
    const LcioEdmTypeMapping& typeMapping,
    const std::vector<std::pair<std::string, EVENT::LCCollection*>>& LCRelation);

  /**
   * Convert a subset collection, dispatching to the correct function for the
   * type of the input collection
   */
  std::unique_ptr<podio::CollectionBase>
  fillSubset(EVENT::LCCollection* LCCollection, const LcioEdmTypeMapping& typeMapping, const std::string& type);

  /*
   * Converts a LCIntVec or LCFloatVec Collection into a podio::UserDataCollection of the appropriate type.
   *
   * NOTE: LC[Int|Float]Vec are nested, but podio::UserDataCollection are flat. Hence, this will put all
   * contents into one collection, and the [begin, end) indices in this collection into a second (flat)
   * collection (with the suffix "_VecLengths" added to its name), such that the elements at position i,
   * resp. (i + 1) form the [begin, end) indices for each of the original vector collections.
   */
  template<typename LCVecType>
  std::vector<CollNamePair> convertLCVec(const std::string& name, EVENT::LCCollection* LCCollection);

  /**
   * Converting all parameters of an LCIO Object and attaching them to the
   * passed podio::Frame.
   */
  template<typename LCIOType>
  void convertObjectParameters(LCIOType* lcioobj, podio::Frame& event);

  inline edm4hep::Vector3f Vector3fFrom(const double* v) { return edm4hep::Vector3f(v[0], v[1], v[2]); }

  inline edm4hep::Vector3f Vector3fFrom(const EVENT::FloatVec& v) { return edm4hep::Vector3f(v[0], v[1], v[2]); }

  /**
   * Convert a TrackState
   */
  edm4hep::TrackState convertTrackState(const EVENT::TrackState* trackState);

  /**
   * Convert a ParticleID object.
   *
   * In LCIO ParticleIDs are persisted as part of the ReconstructedParticle they
   * are attached to. There are no ParticleID collections in the event. Hence,
   * this function converts single particle ID objects and the management of
   * putting them into a collection and of creating the LCIO to EDM4hep mapping
   * is done in the conversion of the ReconstructedParticles.
   */
  edm4hep::MutableParticleID convertPaticleID(const EVENT::ParticleID* pid);

  /**
   * Convert an MCParticle collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   */
  std::unique_ptr<edm4hep::MCParticleCollection> convertMCParticles(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::MCParticle*, edm4hep::MutableMCParticle>& mcparticlesMap);

  /**
   * Convert a ReconstructedParticle collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   *
   * NOTE: Also populates a ParticleID collection, as those are persisted as
   * part of the ReconstructedParticles in LCIO. The name of this collection is
   * <name>_particleIDs
   */
  std::vector<CollNamePair> convertReconstructedParticles(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::ReconstructedParticle*, edm4hep::MutableReconstructedParticle>& recoparticlesMap,
    TypeMapT<const lcio::ParticleID*, edm4hep::MutableParticleID>& particleIDMap);

  /**
   * Convert a Vertex collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   */
  std::unique_ptr<edm4hep::VertexCollection> convertVertices(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::Vertex*, edm4hep::MutableVertex>& vertexMap);

  /**
   * Convert a SimTrackerHit collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   */
  std::unique_ptr<edm4hep::SimTrackerHitCollection> convertSimTrackerHits(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::SimTrackerHit*, edm4hep::MutableSimTrackerHit>& SimTrHitMap);

  /**
   * Convert a TPCHit collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   */
  std::unique_ptr<edm4hep::RawTimeSeriesCollection> convertTPCHits(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::TPCHit*, edm4hep::MutableRawTimeSeries>& TPCHitMap);

  /**
   * Convert a TrackerHit collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   */
  std::unique_ptr<edm4hep::TrackerHitCollection> convertTrackerHits(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::TrackerHit*, edm4hep::MutableTrackerHit>& TrackerHitMap);

  /**
   * Convert a TrackerHitPlane collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   */
  std::unique_ptr<edm4hep::TrackerHitPlaneCollection> convertTrackerHitPlanes(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::TrackerHitPlane*, edm4hep::MutableTrackerHitPlane>& TrackerHitPlaneMap);

  /**
   * Convert a Track collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   */
  std::unique_ptr<edm4hep::TrackCollection> convertTracks(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::Track*, edm4hep::MutableTrack>& TrackMap);

  /**
   * Convert a SimCalorimeterHit collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   */
  std::unique_ptr<edm4hep::SimCalorimeterHitCollection> convertSimCalorimeterHits(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::SimCalorimeterHit*, edm4hep::MutableSimCalorimeterHit>& SimCaloHitMap);

  /**
   * Convert a RawCalorimeterHit collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   */
  std::unique_ptr<edm4hep::RawCalorimeterHitCollection> convertRawCalorimeterHits(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::RawCalorimeterHit*, edm4hep::MutableRawCalorimeterHit>& rawCaloHitMap);

  /**
   * Convert a CalorimeterHit collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   */
  std::unique_ptr<edm4hep::CalorimeterHitCollection> convertCalorimeterHits(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::CalorimeterHit*, edm4hep::MutableCalorimeterHit>& caloHitMap);

  /**
   * Convert a Cluster collection and return the resulting collection.
   * Simultaneously populates the mapping from LCIO to EDM4hep objects.
   *
   * NOTE: Also populates a ParticleID collection, as those are persisted as
   * part of the Cluster collection in LCIO. The name of this collection is
   * <name>_particleIDs
   */
  std::vector<CollNamePair> convertClusters(
    const std::string& name,
    EVENT::LCCollection* LCCollection,
    TypeMapT<const lcio::Cluster*, edm4hep::MutableCluster>& clusterMap,
    TypeMapT<const lcio::ParticleID*, edm4hep::MutableParticleID>& particleIDMap);

  /**
   * Create an EventHeaderCollection and fills it with the Metadata.
   */

  std::unique_ptr<edm4hep::EventHeaderCollection> createEventHeader(const EVENT::LCEvent* evt);

  /**
   * Helper function to create a subset collection from an existing (LCIO)
   * subset collection. Needs the object mapping as input to resolve to the
   * correct EDM4hep object for a given LCIO object.
   *
   * NOTE: Users responsibility to call this with the right inputs (i.e.
   * matching types)
   */
  template<typename CollT, typename LcioT, typename Edm4hepT>
  auto handleSubsetColl(EVENT::LCCollection* lcioColl, const TypeMapT<const LcioT*, Edm4hepT>& elemMap)
  {
    auto edm4hepColl = std::make_unique<CollT>();
    edm4hepColl->setSubsetCollection();

    UTIL::LCIterator<LcioT> lcioIter(lcioColl);
    while (const auto lcioElem = lcioIter.next()) {
      const auto it = elemMap.find(lcioElem);
      if (it != elemMap.end()) {
        edm4hepColl->push_back(it->second);
      }
      else {
        std::cerr << "Cannot find corresponding EDM4hep object for an LCIO object in a subset collection" << std::endl;
      }
    }

    return edm4hepColl;
  }

  namespace detail {
    /// Helper function for generic map lookup
    template<typename LCIOT, typename EDM4hepT>
    std::optional<EDM4hepT> mapLookup(const LCIOT* elem, const TypeMapT<const LCIOT*, EDM4hepT>& map)
    {
      if (const auto& it = map.find(elem); it != map.end()) {
        return std::optional(it->second);
      }
      return std::nullopt;
    }
  } // namespace detail

  /**
   * Create an Association collection from an LCRelations collection. Templated
   * on the From and To types as well as the direction of the relations in the
   * input LCRelations collection with respect to the order in which they are
   * mentioned in the Association collection of EDM4hep (since those are not
   * directed).
   *
   * Necessary inputs apart from the LCRelations collection are the correct LCIO
   * to EDM4hep object mappings to actually resolve the necessary relations.
   */
  template<
    typename CollT,
    bool Reverse,
    typename FromLCIOT,
    typename ToLCIOT,
    typename FromEDM4hepT,
    typename ToEDM4hepT>
  std::unique_ptr<CollT> createAssociationCollection(
    EVENT::LCCollection* relations,
    const TypeMapT<const FromLCIOT*, FromEDM4hepT>& fromMap,
    const TypeMapT<const ToLCIOT*, ToEDM4hepT>& toMap)
  {
    auto assocColl = std::make_unique<CollT>();
    auto relIter = UTIL::LCIterator<EVENT::LCRelation>(relations);

    while (const auto rel = relIter.next()) {
      auto assoc = assocColl->create();
      assoc.setWeight(rel->getWeight());
      const auto lcioTo = static_cast<ToLCIOT*>(rel->getTo());
      const auto lcioFrom = static_cast<FromLCIOT*>(rel->getFrom());
      const auto edm4hepTo = detail::mapLookup(lcioTo, toMap);
      const auto edm4hepFrom = detail::mapLookup(lcioFrom, fromMap);
      if (edm4hepTo.has_value() && edm4hepFrom.has_value()) {
        if constexpr (Reverse) {
          if constexpr (std::is_same_v<ToEDM4hepT, edm4hep::MutableVertex>) {
            assoc.setVertex(*edm4hepTo);
          }
          else {
            assoc.setSim(*edm4hepTo);
          }
          assoc.setRec(*edm4hepFrom);
        }
        else {
          if constexpr (std::is_same_v<FromEDM4hepT, edm4hep::MutableVertex>) {
            assoc.setVertex(*edm4hepFrom);
          }
          else {
            assoc.setSim(*edm4hepFrom);
          }
          assoc.setRec(*edm4hepTo);
        }
      }
    }

    return assocColl;
  }

  /**
   * Creates the CaloHitContributions for all SimCaloHits.
   * has to be done this way, since the converted McParticles are needeed.
   * The contributions are also attached to their corresponding SimCalorimeterHits.
   */
  std::unique_ptr<edm4hep::CaloHitContributionCollection> createCaloHitContributions(
    TypeMapT<const lcio::SimCalorimeterHit*, edm4hep::MutableSimCalorimeterHit>& SimCaloHitMap,
    const TypeMapT<const lcio::MCParticle*, edm4hep::MutableMCParticle>& mcparticlesMap);

  /**
   * Resolve the relations for the MCParticles.
   */
  void resolveRelationsMCParticles(TypeMapT<const lcio::MCParticle*, edm4hep::MutableMCParticle>& mcparticlesMap);

  /**
   * Resolve the relations for SimTrackerHits
   */
  void resolveRelationsSimTrackerHits(
    TypeMapT<const lcio::SimTrackerHit*, edm4hep::MutableSimTrackerHit>& SimTrHitMap,
    const TypeMapT<const lcio::MCParticle*, edm4hep::MutableMCParticle>& mcparticlesMap);

  /**
   * Resolve the relations for ReconstructedParticles
   */
  void resolveRelationsRecoParticles(
    TypeMapT<const lcio::ReconstructedParticle*, edm4hep::MutableReconstructedParticle>& recoparticlesMap,
    const TypeMapT<const lcio::Vertex*, edm4hep::MutableVertex>& vertexMap,
    const TypeMapT<const lcio::Cluster*, edm4hep::MutableCluster>& clusterMap,
    const TypeMapT<const lcio::Track*, edm4hep::MutableTrack>& tracksMap);

  /**
   * Resolve the relations for Clusters
   */
  void resolveRelationsClusters(
    TypeMapT<const lcio::Cluster*, edm4hep::MutableCluster>& clustersMap,
    const TypeMapT<const lcio::CalorimeterHit*, edm4hep::MutableCalorimeterHit>& caloHitMap);

  /**
   * Resolve the relations for Tracks
   */
  void resolveRelationsTracks(
    TypeMapT<const lcio::Track*, edm4hep::MutableTrack>& tracksMap,
    const TypeMapT<const lcio::TrackerHit*, edm4hep::MutableTrackerHit>& trackerHitMap);

  /**
   * Resolve the relations for Vertices
   */
  void resolveRelationsVertices(
    TypeMapT<const lcio::Vertex*, edm4hep::MutableVertex>& vertexMap,
    const TypeMapT<const lcio::ReconstructedParticle*, edm4hep::MutableReconstructedParticle>& recoparticleMap);

  template<typename LCIOType>
  void convertObjectParameters(LCIOType* lcioobj, podio::Frame& event)
  {
    const auto& params = lcioobj->getParameters();
    // handle srting params
    EVENT::StringVec keys;
    const auto stringKeys = params.getStringKeys(keys);
    for (int i = 0; i < stringKeys.size(); i++) {
      EVENT::StringVec sValues;
      const auto stringVals = params.getStringVals(stringKeys[i], sValues);
      event.putParameter(stringKeys[i], stringVals);
    }
    // handle float params
    EVENT::StringVec fkeys;
    const auto floatKeys = params.getFloatKeys(fkeys);
    for (int i = 0; i < floatKeys.size(); i++) {
      EVENT::FloatVec fValues;
      const auto floatVals = params.getFloatVals(floatKeys[i], fValues);
      event.putParameter(floatKeys[i], floatVals);
    }
    // handle int params
    EVENT::StringVec ikeys;
    const auto intKeys = params.getIntKeys(ikeys);
    for (int i = 0; i < intKeys.size(); i++) {
      EVENT::IntVec iValues;
      const auto intVals = params.getIntVals(intKeys[i], iValues);
      event.putParameter(intKeys[i], intVals);
    }
    // handle double params
    EVENT::StringVec dkeys;
    const auto dKeys = params.getDoubleKeys(dkeys);
    for (int i = 0; i < dKeys.size(); i++) {
      EVENT::DoubleVec dValues;
      const auto dVals = params.getDoubleVals(dKeys[i], dValues);
      event.putParameter(dKeys[i], dVals);
    }
  }

  template<typename LCVecType>
  std::vector<CollNamePair> convertLCVec(const std::string& name, EVENT::LCCollection* LCCollection)
  {
    auto dest = std::make_unique<podio::UserDataCollection<typename LCVecType::value_type>>();
    auto vecSizes = std::make_unique<podio::UserDataCollection<uint32_t>>();
    if (LCCollection->getNumberOfElements() > 0) {
      vecSizes->push_back(0);
    }
    for (unsigned i = 0, N = LCCollection->getNumberOfElements(); i < N; ++i) {
      const auto* rval = static_cast<LCVecType*>(LCCollection->getElementAt(i));
      for (unsigned j = 0; j < rval->size(); j++) {
        dest->push_back((*rval)[j]);
      }
      vecSizes->push_back(dest->size());
    }
    std::vector<CollNamePair> results;
    results.reserve(2);
    results.emplace_back(name, std::move(dest));
    results.emplace_back(name + "_VecLenghts", std::move(vecSizes));
    return results;
  }

} // namespace LCIO2EDM4hepConv

#endif // K4EDM4HEP2LCIOCONV_K4LCIO2EDM4HEPCONV_H
