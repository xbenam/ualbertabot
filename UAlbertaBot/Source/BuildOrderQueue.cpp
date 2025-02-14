#include "BuildOrderQueue.h"

using namespace UAlbertaBot;

BuildOrderQueue::BuildOrderQueue()
{

}

void BuildOrderQueue::clearAll()
{
    // clear the queue
    queue.clear();

    // reset the priorities
    highestPriority = 0;
    lowestPriority = 0;
}

BuildOrderItem & BuildOrderQueue::getHighestPriorityItem()
{
    // reset the number of skipped items to zero
    numSkippedItems = 0;

    // the queue will be sorted with the highest priority at the back
    return queue.back();
}

BuildOrderItem & BuildOrderQueue::getNextHighestPriorityItem()
{
    assert(queue.size() - 1 - numSkippedItems >= 0);

    // the queue will be sorted with the highest priority at the back
    return queue[queue.size() - 1 - numSkippedItems];
}

void BuildOrderQueue::skipItem()
{
    // make sure we can skip
    assert(canSkipItem());

    // skip it
    numSkippedItems++;
}

int BuildOrderQueue::numOfSkips()
{
    return numSkippedItems;
}

bool BuildOrderQueue::canSkipItem()
{
    // does the queue have more elements
    bool bigEnough = queue.size() > (size_t)(1 + numSkippedItems);

    auto x = queue[queue.size() - 1 - numSkippedItems];
    auto y = x.metaType;

    bool highestNotBlocking = !queue[queue.size() - 1 - numSkippedItems].blocking;


    if (y.getUnitType().isAddon())
    {
        auto whoNeedThisAddon = y.whatBuilds();
        for (auto& unit : BWAPI::Broodwar->self()->getUnits())
        {
            if (unit->getType() != whoNeedThisAddon) { continue; }
            // Check if build already have an Addon
            if (unit->getAddon() != nullptr) { continue; }
            bool isBlocked = false;

            // if the unit doesn't have space to build an addon, it can't make one
            BWAPI::TilePosition addonPosition(unit->getTilePosition().x + unit->getType().tileWidth(), unit->getTilePosition().y + unit->getType().tileHeight() - y.getUnitType().tileHeight());

            // Fixing addon position
            for (int i = 0; i < y.getUnitType().tileWidth(); ++i)
            {
                for (int j = 0; j < y.getUnitType().tileHeight(); ++j)
                {
                    BWAPI::TilePosition tilePos(addonPosition.x + i, addonPosition.y + j);

                    // if the map won't let you build here, we can't build it
                    if (!BWAPI::Broodwar->isBuildable(tilePos))
                    {
                        isBlocked = true;
                    }

                    for (auto& u : BWAPI::Broodwar->getUnitsOnTile(tilePos.x, tilePos.y))
                    {
                        //std::cout << u->getType().getName() << std::endl;
                        if (u->getType().isBuilding())
                        {
                            return true;
                        }
                    }
                }
            }
            // We found atleast 1 building without Addon
            return false;
        }
        


        
        return true;
    }
    if (y.isTech() && BWAPI::Broodwar->self()->hasResearched(y.getTechType()) || BWAPI::Broodwar->self()->isResearching(y.getTechType()))
    {
        return true;
    }
    if (y.isUpgrade() && BWAPI::Broodwar->self()->getUpgradeLevel(y.getUpgradeType()) > 0 || BWAPI::Broodwar->self()->isUpgrading(y.getUpgradeType()))
    {
        return true;
    }

    if (!bigEnough)
    {
        return false;
    }
    /*if (x.metaType.getUnitType().isAddon() && 
        BWAPI::Broodwar->self()->allUnitCount(x.metaType.whatBuilds()) == BWAPI::Broodwar->self()->allUnitCount(x.metaType.getUnitType()))
    {
        auto a = BWAPI::Broodwar->self()->allUnitCount(x.metaType.whatBuilds());
        auto b = BWAPI::Broodwar->self()->allUnitCount(x.metaType.getUnitType());
        std::cout << "number of builinds:" << a << "number of Addons: " << b << "building ID: " << x.metaType.whatBuilds() <<
            "Addon ID:" << x.metaType.getUnitType() << std::endl;
        return true;
    }*/
    // this tells us if we can skip
    return highestNotBlocking;
}

void BuildOrderQueue::queueItem(BuildOrderItem b)
{
    // if the queue is empty, set the highest and lowest priorities
    if (queue.empty())
    {
        highestPriority = b.priority;
        lowestPriority = b.priority;
    }

    // push the item into the queue
    if (b.priority <= lowestPriority)
    {
        queue.push_front(b);
    }
    else
    {
        queue.push_back(b);
    }

    // if the item is somewhere in the middle, we have to sort again
    if ((queue.size() > 1) && (b.priority < highestPriority) && (b.priority > lowestPriority))
    {
        // sort the list in ascending order, putting highest priority at the top
        std::sort(queue.begin(), queue.end());
    }

    // update the highest or lowest if it is beaten
    highestPriority = (b.priority > highestPriority) ? b.priority : highestPriority;
    lowestPriority  = (b.priority < lowestPriority)  ? b.priority : lowestPriority;
}

void BuildOrderQueue::queueAsHighestPriority(MetaType m, bool blocking, bool gasSteal)
{
    // the new priority will be higher
    int newPriority = highestPriority + defaultPrioritySpacing;

    // queue the item
    queueItem(BuildOrderItem(m, newPriority, blocking, gasSteal));
}

void BuildOrderQueue::queueAsLowestPriority(MetaType m, bool blocking)
{
    // the new priority will be higher
    int newPriority = lowestPriority - defaultPrioritySpacing;

    // queue the item
    queueItem(BuildOrderItem(m, newPriority, blocking));
}

void BuildOrderQueue::removeHighestPriorityItem()
{
    // remove the back element of the vector
    queue.pop_back();

    // if the list is not empty, set the highest accordingly
    highestPriority = queue.empty() ? 0 : queue.back().priority;
    lowestPriority  = queue.empty() ? 0 : lowestPriority;
}

void BuildOrderQueue::removeCurrentHighestPriorityItem()
{
    // remove the back element of the vector
    queue.erase(queue.begin() + queue.size() - 1 - numSkippedItems);

    //assert((int)(queue.size()) < size);

    // if the list is not empty, set the highest accordingly
    highestPriority = queue.empty() ? 0 : queue.back().priority;
    lowestPriority  = queue.empty() ? 0 : lowestPriority;
}

size_t BuildOrderQueue::size()
{
    return queue.size();
}

bool BuildOrderQueue::isEmpty()
{
    return (queue.size() == 0);
}

BuildOrderItem BuildOrderQueue::operator [] (int i)
{
    return queue[i];
}

void BuildOrderQueue::drawQueueInformation(int x, int y)
{
    //x = x + 25;

    if (!Config::Debug::DrawProductionInfo)
    {
        return;
    }

    std::string prefix = "\x04";

    size_t reps = queue.size() < 12 ? queue.size() : 12;

    // for each unit in the queue
    for (size_t i(0); i<reps; i++) {

        prefix = "\x04";

        const MetaType & type = queue[queue.size() - 1 - i].metaType;

        if (type.isUnit())
        {
            if (type.getUnitType().isWorker())
            {
                prefix = "\x1F";
            }
            else if (type.getUnitType().supplyProvided() > 0)
            {
                prefix = "\x03";
            }
            else if (type.getUnitType().isRefinery())
            {
                prefix = "\x1E";
            }
            else if (type.isBuilding())
            {
                prefix = "\x11";
            }
            else if (type.getUnitType().groundWeapon() != BWAPI::WeaponTypes::None || type.getUnitType().airWeapon() != BWAPI::WeaponTypes::None)
            {
                prefix = "\x06";
            }
        }

        BWAPI::Broodwar->drawTextScreen(x, y+(i*10), " %s%s", prefix.c_str(), type.getName().c_str());
    }
}

bool UAlbertaBot::BuildOrderQueue::isInQueue(MetaType m)
{
    for (auto it : queue)
    {
        if (m.getUnitType() == it.metaType.getUnitType())
            return true;
    }
    return false;
}
