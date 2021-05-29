package agents

import agents.logic.HyperResolutionRule
import akka.actor.typed.{ActorRef, Behavior}
import akka.actor.typed.receptionist.{Receptionist, ServiceKey}
import akka.actor.typed.scaladsl.{ActorContext, Behaviors, LoggerOps}
import constraints.ConflictAvoidingArgument
import exception.Exceptions.NoBacktrackingRequiredException

import scala.annotation.tailrec
import org.slf4j.{Logger, LoggerFactory}

case class Nogood(positions: Map[Int, Int]) {

  /**
   * Check if the current NoGood instance is compatible with the provided Agent View
   *
   * @param agentView a mapping from row indices to column indices
   * @return true if the Nogood is compatible with the Agent View, false otherwise
   */
  def checkCompatibility(agentView: Map[Int, Int]): Boolean =
    if (!(positions.keySet subsetOf agentView.keySet)) {
      false
    }
    else {
      positions.forall { case (key, value) => agentView(key) equals value }
    }

  def isEmpty: Boolean = positions.isEmpty

  def getLowestPriorityAgentId: Int = positions.keySet.max
}

object QueenAgent {


  sealed trait QueenMessageT

  case class QueenMessageAnswerOk() extends QueenMessageT

  case class QueenMessageAskOk(rowId: Int, colId: Int) extends QueenMessageT

  case class QueenMessageAnswerNogood(nogood: Nogood, senderId: Int) extends QueenMessageT

  case class QueenMessageAddLink(senderId: Int) extends QueenMessageT

  case class QueenMessageNoSolution() extends QueenMessageT

  sealed case class ListingResponse(listing: Receptionist.Listing) extends QueenMessageT

  case class FoundAllAgents(agentId: Int) extends QueenMessageT

  import constraints.Constraint._

  implicit val logger: Logger = LoggerFactory.getLogger(LoggerName)

  def getQueenId(rowId: Int): String = s"queen$rowId"

  val queenServiceKeyString: Int => String = queenId => s"queenServiceKey$queenId"
  val queenServiceKey: Int => ServiceKey[QueenMessageT] = queenId =>
    ServiceKey[QueenMessageT](queenServiceKeyString(queenId))

  val listingAdapter: ActorContext[QueenMessageT] => ActorRef[Receptionist.Listing] =
    context => context.messageAdapter[Receptionist.Listing](ListingResponse)

  sealed case class QueenState(
                                context: ActorContext[QueenMessageT],
                                currentRow: Int,
                                currentCol: Int,
                                agentView: LocalView,
                                communicatedNogoods: Array[Nogood],
                                neighbours: Set[Int]
                              ) {
    def changeCol(newCol: Int): QueenState =
      QueenState(context, currentRow, newCol, agentView + (currentRow -> newCol),
        communicatedNogoods, neighbours)

    def changeAgentValue(rowId: Int, colId: Int): QueenState =
      QueenState(context, currentRow, currentCol, agentView + (rowId -> colId),
        communicatedNogoods, neighbours)

    def addNogood(newNogood: Nogood): QueenState = QueenState(context,
      currentRow, currentCol, agentView, communicatedNogoods :+ newNogood, neighbours)

    def addNeighbor(neighbor: Int): QueenState = QueenState(context,
      currentRow, currentCol, agentView, communicatedNogoods, neighbours + neighbor)

  }

  /**
   * Find a new position for the calling queen that is consistent with the provided local view
   *
   * @param agentView An agent view that doesn't contain the position of the calling queen
   * @param numRows   The number of rows in the problem
   * @param nogoods   The list of accumulated nogoods
   * @param rowId     The row id of the calling queen
   * @return a Some encapsulating the new position, or None if no acceptable solution could be found
   */
  def findAcceptableSolution(agentView: LocalView, numRows: Int, nogoods: Nogoods, rowId: Int): Option[Int] =
    Range(0, numRows).find { newPotentialColumn =>
      acceptableSettlement(nogoods)(rowId, newPotentialColumn) apply agentView
    }

  /**
   * Check if the current agent view is not compatible with all the nogoods collected up until now.
   *
   * @param nogoods The list of nogoods that have been received / generated up to this point
   * @return true if no nogood compatible with the current local view could be found, and false otherwise
   */
  def checkAgentViewIncompatibility(nogoods: Nogoods, rowId: Int): LocalView => Boolean = agentView =>
    nogoods filter {
      _.positions.keySet contains rowId
    } forall {
      currentNoGood => !currentNoGood.checkCompatibility(agentView)
    }

  /**
   * Check if the position of the calling agent satisfies all the constraints with respect to the associated
   * local view
   *
   * @param rowId the row number associated with the calling agent
   * @param colId the current column value assignment of the calling agent
   * @return
   */
  def constraintsAreSatisfied(rowId: Int, colId: Int): LocalView => Boolean = agentView =>
    agentView forall {
      case (otherRowId, otherColId) if otherRowId != rowId =>
        ConflictAvoidingArgument(otherRowId, otherColId, rowId, colId).checkConstraint
      case _ => true
    }

  /**
   * Check if the current value assignment (position) can lead to a possible solution
   *
   * @param rowId      the row associated with the calling agent
   * @param queenState the current state of the calling agent
   * @return a function that takes the local view as an argument and returns true if it can lead to a
   *         solution, or false otherwise
   */
  def acceptableSettlement(nogoods: Nogoods)(rowId: Int, colId: Int): LocalView => Boolean = agentView =>
    (checkAgentViewIncompatibility(nogoods, rowId) apply
      agentView + (rowId -> colId)) && (constraintsAreSatisfied(rowId, colId) apply agentView)

  /**
   * Perform a backtracking step: Search for a new column position assignment; If one is found, then change the
   * current value to the new value; otherwise, generate a new nogood using the hyper-resolution rule and send it
   * to the lowest priority agent from it.
   *
   * @param newQueenState the most recent queen state of the calling agent
   * @param currentRow    the id of the calling agent, which is associated with its row number
   * @param numRows       The number of rows from the problem
   * @param queenRegistry The actor references of all the agents and their Ids
   * @return a new Behavior
   */
  def backtrack(context: ActorContext[QueenMessageT], newQueenState: QueenState, currentRow: Int, numRows: Int,
                queenRegistry: YellowBook): Behavior[QueenMessageT] = {
    val agentViewToTest: LocalView = newQueenState.agentView.view.filterKeys(_ != currentRow).toMap
    findAcceptableSolution(agentViewToTest, numRows, newQueenState.communicatedNogoods, currentRow) match {
      case Some(newColumnPosition) =>
        val newQueenState2: QueenState = newQueenState.changeCol(newColumnPosition)
        context.log.debug(s"Found possible acceptable solution $newColumnPosition in the view: $agentViewToTest; new agent view: ${newQueenState2.agentView}")
        /* Send the new value as an Ok? message to all the lower priority queens connected: */
        newQueenState.neighbours.foreach { neighbourId =>
          context.log.debug(s"Send Ok($currentRow, $newColumnPosition) to $neighbourId")
          queenRegistry(neighbourId) ! QueenMessageAskOk(currentRow, newColumnPosition)
        }
        processMessages(
          currentRow,
          numRows,
          newQueenState2,
          queenRegistry
        )
      case None =>
        context.log.debug("No acceptable solution could be found; Emit a nogood;")
        /* Create a new Nogood using the hyper-resolution rule: */
//        val newNogood: Nogood = applyHyperResolutionRule(
//          Range(0, numRows).toList,
//          newQueenState.communicatedNogoods, currentRow, newQueenState.agentView).head
        val newNogood: Nogood = new HyperResolutionRule(Range(0, numRows).toArray, newQueenState.communicatedNogoods,
              currentRow, newQueenState.agentView).applyHyperResolutionRule.head
        context.log.debug(s"Found nogood: $newNogood")
        /* Communicate the nogood to the lowest priority queen from the nogood: */
        queenRegistry(newNogood.getLowestPriorityAgentId) ! QueenMessageAnswerNogood(newNogood, currentRow)
        if (newNogood.isEmpty) {
          /* Broadcast to all agents that there is no available
          * solution for this problem: */
          queenRegistry.foreach { case (agentId, actorRef) if agentId != currentRow =>
            actorRef ! QueenMessageNoSolution()
          }
          /* Terminate: */
          Behaviors.stopped
        }
        else {
          processMessages(
            currentRow,
            numRows,
            newQueenState,
            queenRegistry
          )
        }
    }
  }

  /**
   * The behavior responsible for finding all the available queens
   *
   * @param context    The actor context that corresponds to this queen
   * @param currentRow The row index associated with this queen
   * @param numRows    The number of rows on the current chessboard
   * @return The conflict solving behavior if all the queens have been found, and this same behavior
   *         otherwise
   */
  def findingQueensBehavior(
                             context: ActorContext[QueenMessageT],
                             currentRow: Int,
                             numRows: Int,
                             queenYellowBook: YellowBook,
                             finishedAgents: Int
                           ): Behavior[QueenMessageT] =
    Behaviors.setup {
      context =>
        context.setLoggerName(LoggerName)
        Behaviors.receiveMessage {
          case ListingResponse(listing) =>
            Range(0, numRows)
              .find(rowId => listing.key.id.equals(queenServiceKeyString(rowId))) match {
              case Some(rowToAdd) =>
                val serviceInstances: Set[ActorRef[QueenMessageT]] = listing.allServiceInstances(queenServiceKey(rowToAdd))
                if (serviceInstances.isEmpty) {
                  context.log.debug(s"somehow, the agent ${rowToAdd} is and is not in the listing; Try again")
                  context.system.receptionist ! Receptionist.Find(queenServiceKey(rowToAdd), listingAdapter(context))
                  findingQueensBehavior(context, currentRow, numRows, queenYellowBook, finishedAgents)
                }
                else {
                  val newYellowBook: YellowBook = queenYellowBook + (rowToAdd -> serviceInstances.head)
                  context.log.debug(s"found agent ${rowToAdd}, new YB size: ${newYellowBook.size}")
                  if (newYellowBook.size == numRows) {
                    context.log.debug("I finished")

                    /**
                     * Send the Ok? message to the lower neighbour, if the current queen doesn't have the lowest
                     * priority:
                     */
                    Range(currentRow + 1, numRows).foreach {
                      newYellowBook(_) ! QueenMessageAskOk(rowId = currentRow, colId = 0)
                    }
                    processMessages(
                      currentRow, numRows,
                      QueenState(
                        context,
                        currentRow = currentRow,
                        currentCol = 0,
                        agentView = Map(currentRow -> 0),
                        communicatedNogoods = EmptyNogoods,
                        neighbours = Range(currentRow + 1, numRows).toSet,
                      ), newYellowBook)
                  }
                  else {
                    findingQueensBehavior(context, currentRow, numRows, newYellowBook, finishedAgents)
                  }
                }
              case None =>
                context.log.info("The subscribed agent could not be identified")
                findingQueensBehavior(context, currentRow, numRows, queenYellowBook, finishedAgents)
            }
          case message =>
            context.log.debug(s"$currentRow has received $message ahead of time")
            findingQueensBehavior(context, currentRow, numRows, queenYellowBook, finishedAgents)
        }
    }

  def apply(rowId: Int, numberOfQueens: Int): Behavior[QueenMessageT] = Behaviors.setup { context =>
    context.log.info(s"queen ${rowId} started")
    context.system.receptionist ! Receptionist.Register(queenServiceKey(rowId), context.self)
    Range(0, numberOfQueens).foreach { otherRow =>
      context.system.receptionist ! Receptionist.Find(queenServiceKey(otherRow), listingAdapter(context))
    }
    findingQueensBehavior(context, rowId, numberOfQueens, Map.empty[Int, ActorRef[QueenMessageT]], 0)
  }

  /**
   * Perform the actual Asynchronous Backtracking Algorithm.
   *
   * @param currentRow    the row associated with the calling agent
   * @param numRows       the number of rows in the current environment
   * @param queenRegistry a Map that links a row number to the corresponding queen agent (the calling
   *                      agent is missing)
   * @param queenState    the current state of the calling agent
   * @return
   */
  def processMessages(currentRow: Int,
                      numRows: Int,
                      queenState: QueenState,
                      queenRegistry: YellowBook): Behavior[QueenMessageT] =
    Behaviors.receiveMessage {
      case QueenMessageAskOk(otherRowId, otherColId) =>
        queenState.context.log.debug(s"$currentRow has received QueenMessageAskOk($otherRowId, $otherColId); old queen agent: ${queenState}")
        /* Check if the value of the receiving queen is consistent with its new agent view: */
        val newQueenState: QueenState = queenState.changeAgentValue(otherRowId, otherColId)
        queenState.context.log.debug(s"new queen agent: ${newQueenState}")
        if (!(acceptableSettlement(newQueenState.communicatedNogoods)
        (currentRow, newQueenState.currentCol) apply newQueenState.agentView)) {
          /* Search for another value from this domain that is consistent with the new agent view
          * (except for the row of the calling queen)
          * */
          newQueenState.context.log.debug("proceed with backtrack")
          backtrack(newQueenState.context, newQueenState, currentRow, numRows, queenRegistry)
        }
        else {
          newQueenState.context.log.debug("The current position is fine")
          processMessages(currentRow, numRows, newQueenState, queenRegistry)
        }
      case QueenMessageAnswerNogood(nogood, senderId) =>
        queenState.context.log.debug(s"$currentRow received another nogood message $nogood from $senderId")
        /* add the new nogood to the current collection: */
        val newQueenState: QueenState =
          nogood.positions
            .foldLeft(queenState.addNogood(nogood)) {
              case (acc, (queenId, colVal)) =>
                if (!acc.neighbours.contains(queenId)) {
                  queenRegistry(queenId) ! QueenMessageAddLink(currentRow)
                  acc
                    .changeAgentValue(queenId, colVal)
                } else {
                  acc
                }
            }
        if (!nogood.checkCompatibility(newQueenState.agentView)) {
          queenState.context.log.debug(s"$currentRow is not compatible with the received nogood;")
          queenRegistry(senderId) ! QueenMessageAskOk(rowId = currentRow,
            colId = newQueenState.currentCol)
          processMessages(
            currentRow,
            numRows,
            newQueenState,
            queenRegistry
          )
        }
        else {
          queenState.context.log.debug(s"$currentRow is compatible with the new nogood; Backtrack")
          /* Search for a new, acceptable column position assignment: */
          backtrack(queenState.context, newQueenState, currentRow, numRows, queenRegistry)
        }
      case QueenMessageNoSolution() =>
        queenState.context.log.info(s"Agent $currentRow has received a QueenMessageNoSolution message; terminate;")
        Behaviors.stopped
      case QueenMessageAddLink(senderId) =>
        queenState.context.log.info(s"Agent $currentRow has received a QueenMessageAddLink($senderId) message;")
        /* Add a link from currentRow to senderId; */
        val newQueenState: QueenState = queenState.addNeighbor(senderId)
        queenRegistry(senderId) ! QueenMessageAskOk(rowId = currentRow, colId = queenState.currentCol)
        processMessages(currentRow, numRows, newQueenState, queenRegistry)
    }


}
