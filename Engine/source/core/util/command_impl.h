//-----------------------------------------------------------------------------
/// Copyright (c) 2013-2017 BitBox, Ltd.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------
#pragma once

#ifndef _CORE_UTIL_COMMAND_DETAILS_H_
#define _CORE_UTIL_COMMAND_DETAILS_H_

#include <tuple>
#include <utility>
#include <future>

namespace Command
{

enum class Result
{
	Finished,
	RolledBack
};

namespace Details
{

namespace Task
{

/**
 * Tuples that represent data that should be passed to the execution functor
 * of next task and to the rollback functor of the current task are marked with
 * dummy structs to ease static validation of functor signatures
 */
struct ForNextTaskTupleTag {};
struct ForRollbackTupleTag {};

template <typename... T>
using ForNextTaskTuple = std::tuple<ForNextTaskTupleTag, std::decay_t<T>...>;

template <typename... T>
using ForRollbackTuple = std::tuple<ForRollbackTupleTag, std::decay_t<T>...>;

/**
* Execution functor return type of every task has the following structure:
* std::tuple<bool, ForNextTaskTuple<...>, ForRollbackTuple<...>>
*/
template <typename TaskType>
using Result = StdX::ResultType<typename std::remove_reference_t<TaskType>::ExecutionFunc>;

/**
 * Actual input for task functors differs from @see ForNextTaskTuple and
 * @see ForRollbackTuple because their first elements are just dummy tags
 * to ease static analysis
 */
template <typename TaskType>
using InputForNextTask = StdX::TupleTailType<std::tuple_element_t<1, Result<TaskType>>>;

template <typename TaskType>
using InputForRollback = StdX::TupleTailType<std::tuple_element_t<2, Result<TaskType>>>;

template <typename Execute, typename Rollback, typename ThreadMoverType>
class GenericTask final
{
public:
	using ExecutionFunc = std::decay_t<Execute>;
	using RollbackFunc = std::decay_t<Rollback>;
	using ThreadMover = ThreadMoverType;

	GenericTask(Execute&& executeFunc, Rollback&& rollbackFunc, ThreadMover&& threadMover)
		: executeFunc(std::forward<Execute>(executeFunc))
		, rollbackFunc(std::forward<Rollback>(rollbackFunc))
		, threadMover(std::forward<ThreadMover>(threadMover)) {}

	ExecutionFunc executeFunc;
	RollbackFunc rollbackFunc;
	ThreadMover threadMover;

private:
	template <typename... T>
	struct IsValidResult : std::false_type {};

	template <
		template <typename...> class T1, typename... Types1,
		template <typename...> class T2, typename... Types2
	>
	struct IsValidResult<std::tuple<bool, T1<Types1...>, T2<Types2...>>>
	{
		using ForNext = T1<Types1...>;
		using ForRollback = T2<Types2...>;

		static constexpr bool NextIsValid
			= std::conjunction<
				StdX::Traits::IsTuple<ForNext>,
				std::is_same<ForNextTaskTupleTag, std::tuple_element_t<0, ForNext>>
			>::value;
		static constexpr bool RollbackIsValid
			= std::conjunction<
				StdX::Traits::IsTuple<ForRollback>,
				std::is_same<ForRollbackTupleTag, std::tuple_element_t<0, ForRollback>>
			>::value;
		static constexpr bool value = NextIsValid && RollbackIsValid;
	};

	static_assert(StdX::Traits::IsCallableEntity<ExecutionFunc>::value
		&& StdX::Traits::IsCallableEntity<RollbackFunc>::value,
		"GenericTask template arguments must be callable entities (note that "
		"generic lambda-s are not supported)");

	using ResultType = StdX::ResultType<ExecutionFunc>;
	static_assert(IsValidResult<ResultType>::value,
		"Execute functor has invalid return type structure, use TaskResult() "
		"function in a return statement");

	using RollbackInput = StdX::TupleTailType<std::tuple_element_t<2, StdX::ResultType<ExecutionFunc>>>;

	static_assert(StdX::Traits::IsAppliable<Rollback, RollbackInput>::value,
		"Invalid function signature of Task components: Rollback functor can't "
		"be called with output of Execute functor");
};

template <typename T>
struct IsTask : std::false_type {};

template <typename Execute, typename Rollback, typename ThreadPusher>
struct IsTask<GenericTask<Execute, Rollback, ThreadPusher>> : std::true_type {};

} // namespace Task

/**
 * @brief An object that represents a Command object
 * @details Stores inside it all the tasks and all the data that is
 * returned by execution functors of these tasks. Fully implements the core
 * of command execution logic in itself
 */
template <typename... Tasks>
class Cmd final
{
public:
	static_assert(StdX::Traits::AllOf<Task::IsTask, std::remove_cv_t<std::remove_reference_t<Tasks>>...>::value,
		"Only GenericTask objects may be passed to Command. Use helper functions "
		"like Command::Task::MainThread() to generate them");

	Cmd(Tasks&& ... tasks)
		: mTasks(std::forward<Tasks>(tasks)...)
	{
	};

	Cmd(Cmd&&) = default;
	Cmd& operator=(Cmd&&) = default;

	Cmd(const Cmd&) = delete;
	Cmd& operator=(const Cmd&) = delete;

	static std::future<Result> StartExecution(Cmd cmd)
	{
		auto ret = cmd.mExecutionFinished.get_future();
		ContinueExecution(std::move(cmd));
		return ret;
	}

	static void ContinueExecution(Cmd cmd)
	{
		if (cmd.mRollingBack)
		{
			_rollback(std::move(cmd));
			return;
		}

		while (auto task = cmd._getNextUnperformedTask())
		{
			if (task->isInTargetThread(cmd))
			{
				bool taskSuccessful = task->execute(&cmd);
				if (!taskSuccessful)
				{
					_rollback(std::move(cmd));
					return;
				}

				++cmd.mNextTask;
			}
			else
			{
				task->moveToTargetThread(std::move(cmd));
				return;
			}
		}

		cmd.mExecutionFinished.set_value(Result::Finished);
	}

private:
	class ITaskExecutor
	{
	public:
		virtual ~ITaskExecutor() = default;

		virtual bool execute(Cmd*) const = 0;
		virtual void rollback(Cmd*) const = 0;
		virtual void moveToTargetThread(Cmd) const = 0;
		virtual bool isInTargetThread(const Cmd&) const = 0;
	};

	template <size_t TaskNum> class TaskExecutor;

	static constexpr size_t TasksCount = sizeof...(Tasks);

	template <size_t... Indices>
	static auto _makeExecutorsImpl(std::index_sequence<Indices...>)
	{
		return std::make_tuple(TaskExecutor<Indices>()...);
	}

	static auto _makeExecutors()
	{
		return _makeExecutorsImpl(std::make_index_sequence<TasksCount>());
	}

	template <size_t... Indices>
	static auto _makeIteratableExecutorsImpl(std::index_sequence<Indices...>)
	{
		return std::array<const ITaskExecutor*, TasksCount> {(&std::get<Indices>(Executors))...};
	}

	static auto _makeIteratableExecutors()
	{
		return _makeIteratableExecutorsImpl(std::make_index_sequence<TasksCount>());
	}

	const ITaskExecutor* _getNextUnperformedTask()
	{
		return mNextTask < IteratableExecutors.size() ? IteratableExecutors[mNextTask] : nullptr;
	}

	const ITaskExecutor* _getLastPerformedTask()
	{
		return mNextTask > 0 ? IteratableExecutors[mNextTask - 1] : nullptr;
	}

	static void _rollback(Cmd cmd)
	{
		cmd.mRollingBack = true;
		while (auto task = cmd._getLastPerformedTask())
		{
			if (task->isInTargetThread(cmd))
			{
				task->rollback(&cmd);
				--cmd.mNextTask;
			}
			else
			{
				task->moveToTargetThread(std::move(cmd));
				return;
			}
		}

		cmd.mRollingBack = false;
		cmd.mExecutionFinished.set_value(Result::RolledBack);
	}

	std::promise<Result> mExecutionFinished;
	bool mRollingBack = false;
	size_t mNextTask = 0;
	std::tuple<std::remove_reference_t<Tasks>...> mTasks;
	std::tuple<Task::InputForNextTask<Tasks>...> mInputForNextTask;
	std::tuple<Task::InputForRollback<Tasks>...> mRollbackInput;
	/**
	 * @see TaskExecutor for info about what this is. Since TaskExecutor-s
	 * are essentially just a code-generation utility that do not store any
	 * data, they can be stored as a static const object. This allows to not worry
	 * about availability of TaskExecutor-s in different threads that execute
	 * tasks
	 */
	static const decltype(_makeExecutors()) Executors;
	/**
	 * Array of pointers to @see Executors elements
	 */
	static const std::array<const ITaskExecutor*, TasksCount> IteratableExecutors;
};

/**
 * @details Generic nature of Command object means that functors and data
 * they generate/consume all have different types. Different types mean that
 * a regular homogeneous container (e.g. std::vector) can't be used for storage.
 *
 * Though std::tuple may be used instead, the following problem arises:
 * how to iterate over functor with different types that accept
 * input of different types in a statically typed language? Also, since
 * tasks may be executed in different threads, ability to make deferred
 * iteration is required (first iteration in one thread, second iteration
 * later in a different thread, etc.).
 *
 * The solution that is used here is in generating a separate TaskExecutor
 * object for each iteration. The index of iteration is used as a template
 * parameter of TaskExecutor which allows to retrieve data from tuple-s via
 * std::get<>().
 *
 * TaskExecutor-s have a common abstract base class ITaskExecutor, pointers
 * to which may be stored in a regular homogeneous containers that is easy
 * to iterate over.
 */
template <typename... Tasks>
template <size_t TaskNumber>
class Cmd<Tasks...>::TaskExecutor final : public ITaskExecutor
{
public:
	bool execute(Cmd* cmd) const final
	{
		auto& executeFunc = std::get<TaskNumber>(cmd->mTasks).executeFunc;
		auto& executionInput = _getTaskInput<TaskNumber>(cmd);
		auto result = StdX::Apply(executeFunc, std::move(executionInput));
		bool success = std::get<0>(result);

		if (success)
		{
			_getTaskInput<TaskNumber + 1>(cmd) = StdX::TupleTail(std::get<1>(std::move(result)));
			_getRollbackInput<TaskNumber>(cmd) = StdX::TupleTail(std::get<2>(std::move(result)));
		}

		return success;
	}

	void rollback(Cmd* cmd) const final
	{
		auto& rollbackFunc = std::get<TaskNumber>(cmd->mTasks).rollbackFunc;
		auto& rollbackInput = _getRollbackInput<TaskNumber>(cmd);
		StdX::Apply(rollbackFunc, std::move(rollbackInput));
	}

	void moveToTargetThread(Cmd cmd) const final
	{
		_getThreadMover(cmd).moveToTargetThread(std::move(cmd));
	}

	bool isInTargetThread(const Cmd& cmd) const final
	{
		return _getThreadMover(cmd).isInTargetThread();
	}

private:
	struct ExecutionFunctorHasValidSignature
	{
	private:
		template <size_t TaskNum>
		using TaskType = std::remove_reference_t<StdX::ParameterPackNthType<TaskNum, Tasks...>>;

		template <size_t TaskNum>
		struct InputForThisTask
		{
			using PreviousTaskType = TaskType < TaskNum - 1 >;
			using Type = Task::InputForNextTask<PreviousTaskType>;
		};

		template <>
		struct InputForThisTask<0>
		{
			using Type = std::tuple<>;
		};

		using ThisTaskExecutionFunc = typename TaskType<TaskNumber>::ExecutionFunc;

	public:
		static constexpr bool value
			= StdX::Traits::IsAppliable<
				ThisTaskExecutionFunc,
				typename InputForThisTask<TaskNumber>::Type
			>::value;
	};

	static_assert(ExecutionFunctorHasValidSignature::value,
		"Execution functor signatures of two adjacent Tasks do not match. "
		"Possability 1: execution functor of the first Task has non-empty parameter list. "
		"Possability 2: ForNextTask(...) contents of the return statement of a previous "
		"Tasks's execution functor do not match the input parameter list of "
		"current Task's execution functor");

	template <size_t TaskNumber>
	auto& _getTaskInput(Cmd* cmd) const
	{
		return std::get<TaskNumber - 1>(cmd->mInputForNextTask);
	}

	template <>
	auto& _getTaskInput<0>(Cmd* cmd) const
	{
		static const std::tuple<> Empty;
		return Empty;
	}

	template <size_t TaskNumber>
	auto& _getRollbackInput(Cmd* cmd) const
	{
		return std::get<TaskNumber>(cmd->mRollbackInput);
	}

	const auto& _getThreadMover(const Cmd& cmd) const
	{
		return std::get<TaskNumber>(cmd.mTasks).threadMover;
	}
};

#pragma warning(push)
#pragma warning(disable:4592) // MSVC2015: symbol will be dynamically initialized (implementation limitation)
template <typename... Tasks>
/*static*/ const decltype(Cmd<Tasks...>::_makeExecutors()) Cmd<Tasks...>::Executors = _makeExecutors();

template <typename... Tasks>
/*static*/ const decltype(Cmd<Tasks...>::_makeIteratableExecutors()) Cmd<Tasks...>::IteratableExecutors = _makeIteratableExecutors();
#pragma warning(pop)

} // namespace Details

template <typename... Tasks>
Details::Cmd<Tasks...> Make(Tasks&&... tasks)
{
	return Details::Cmd<Tasks...>(std::forward<Tasks>(tasks)...);
}

template <typename... Tasks>
std::future<Result> Execute(Details::Cmd<Tasks...> cmd)
{
	return Details::Cmd<Tasks...>::StartExecution(std::move(cmd));
}

} // namespace Command

#endif // _CORE_UTIL_COMMAND_DETAILS_H_
